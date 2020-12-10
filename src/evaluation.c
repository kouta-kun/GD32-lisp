#include "ev_header.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

struct function_table funtable;

extern char *EV_LOG;
static char mylog[512];

static struct table_entry *get_entry(struct function_table *table, struct se_symbol symbol)
{
    if (table->entry_count < 1)
    {
        strcpy(mylog, "Low entry count: ");
        sprintf(mylog + strlen(mylog), "%d", table->entry_count);
        return NULL;
    }

    char symname[7];
    memset(symname, 0, 7);
    memcpy(symname, symbol.sym, symbol.sym_size);

    uint8_t keyBytesH[4];
    memcpy(keyBytesH, symname, 4);
    uint8_t keyBytesL[4];
    memcpy(keyBytesL, symname + 4, 3);
    keyBytesL[3] = symbol.sym_size;

    uint32_t hKey = *(uint32_t *)keyBytesH;
    uint32_t lKey = *(uint32_t *)keyBytesL;
    uint32_t key = hKey + lKey;

    struct table_entry *node = table->entries;
    while (node != NULL)
    {
        if (node->key == key)
            return node;
        else if (node->key > key)
            node = node->left;
        else
            node = node->right;
    }
    if (node == NULL)
    {
        strcpy(mylog, "Failed to find entry for key: ");
        sprintf(mylog + strlen(mylog), "%lu.", key);
    }
    return node;
}

static int create_entry(struct function_table *table, struct table_entry **reference_entry)
{
    int entryIndex = table->entry_count++;
    if (table->entry_count > table->entry_size)
    {
        int oldSize = table->entry_size;
        table->entry_size *= 2;
        struct table_entry *newEntries = calloc(sizeof(struct table_entry), (table->entry_size));
        struct table_entry *oldEntries = table->entries;
        memcpy(newEntries, oldEntries, sizeof(struct table_entry) * (oldSize));
        for (int i = 0; i < oldSize; i++)
        {
            if (newEntries[i].left != NULL)
            {
                newEntries[i].left = (newEntries[i].left - oldEntries) + newEntries;
            }
            if (newEntries[i].right != NULL)
            {
                newEntries[i].right = (newEntries[i].right - oldEntries) + newEntries;
            }
        }
        *reference_entry = (*reference_entry) - oldEntries + newEntries;
        free(table->entries);
        table->entries = newEntries;
    }
    return entryIndex;
}

static struct table_entry *put_entry(struct function_table *table, struct se_symbol symbol, se_function function)
{
    char symname[7];
    memset(symname, 0, 7);
    memcpy(symname, symbol.sym, symbol.sym_size);

    uint8_t keyBytesH[4];
    memcpy(keyBytesH, symname, 4);
    uint8_t keyBytesL[4];
    memcpy(keyBytesL, symname + 4, 3);
    keyBytesL[3] = symbol.sym_size;

    uint32_t hKey = *(uint32_t *)keyBytesH;
    uint32_t lKey = *(uint32_t *)keyBytesL;
    uint32_t key = hKey + lKey;

    if (table->entry_count < 1)
    {
        table->entries[0] = (struct table_entry){.key = key};
        table->entry_count++;
    }

    sprintf(mylog + strlen(mylog), "(hash for %s:%lu)", symname, key);

    struct table_entry *destNode = table->entries;
    while (destNode->function != NULL)
    {
        if (destNode->key == key)
        {
            destNode->function = NULL;
            EV_LOG = "Replaced at least one function while starting up\r\n";
        }
        else if (destNode->key > key)
        {
            if (destNode->left == NULL)
            {
                int entryIndex = create_entry(table, &destNode);
                destNode->left = table->entries + entryIndex;
                destNode->left->key = key;
            }
            destNode = destNode->left;
        }
        else
        {
            if (destNode->right == NULL)
            {
                int entryIndex = create_entry(table, &destNode);
                destNode->right = table->entries + entryIndex;
                destNode->right->key = key;
            }
            destNode = destNode->right;
        }
    }
    destNode->function = function;
    return destNode;
}

struct se_node *list_index_ptr(struct se_list *list, int index)
{
    while (index-- > 0)
    {
        list = &list->next->list_val;
    }
    return list->element;
}

struct se_node *matrix_index_ptr(struct se_matrix matrix, int row, int column)
{
    if (column >= matrix.columns)
    {
        EV_LOG = "Column index out of range.";
        return NULL;
    }
    if (row >= matrix.rows)
    {
        EV_LOG = "Row index out of range.";
        return NULL;
    }

    return list_index_ptr(&matrix.list->list_val, row * matrix.columns + column);
}

int get_list_size(struct se_list *list)
{
    int list_size = 0;
    while (list != NULL)
    {
        list_size++;
        list = &list->next->list_val;
    }
    return list_size;
}

struct se_node matget(struct se_list *list)
{
    struct se_node element = eval_object(list->element);
    if (element.tag != MATRIX)
    {
        sprintf(mylog, "Tried to matrix-index a non-matrix object!");
        log_object(&element, mylog);
        EV_LOG = mylog;
        return (struct se_node){.tag = LIST, .list_val = {.element = NULL, .next = NULL}};
    }
    int row = list->next->list_val.element->int_val;
    int column = list->next->list_val.next->list_val.element->int_val;
    return *(matrix_index_ptr(element.matrix_val, row, column));
}

struct se_node asmat(struct se_list *list)
{
    struct se_node *source_list = list->element;
    int rows = list->next->list_val.element->int_val;
    int columns = list->next->list_val.next->list_val.element->int_val;
    int lsize = get_list_size(&source_list->list_val);
    if (rows * columns != lsize)
    {
        sprintf(mylog, "Tried to make a matrix from a non-conforming list: (%d*%d) != %d\r\n", rows, columns, lsize);
        EV_LOG = mylog;
        return (struct se_node){.tag = LIST, .list_val = {.element = NULL, .next = NULL}};
    }
    return (struct se_node){.tag = MATRIX, .matrix_val = {.rows = rows, .columns = columns, .list = source_list}};
}

struct se_node listget(struct se_list *list)
{
    struct se_node element = eval_object(list->next->list_val.element);
    if (element.tag != NUMBER)
    {
        EV_LOG = "List index must be a number.";
        return (struct se_node){.tag = LIST, .list_val = {.element = NULL, .next = NULL}};
    }
    int list_index = element.int_val;
    struct se_node list_element = eval_object(list->element);
    if (list_element.tag != NUMBER)
    {
        EV_LOG = "Tried to access a nonlist object.";
        return (struct se_node){.tag = LIST, .list_val = {.element = NULL, .next = NULL}};
    }
    return *(list_index_ptr(&list_element.list_val, list_index));
}

struct se_node listsize(struct se_list *list)
{
    struct se_node element = eval_object(list->element);
    if (element.tag != LIST)
    {
        EV_LOG = "Tried to get size of a nonlist object";
        return (struct se_node){.tag = LIST, .list_val = {.element = NULL, .next = NULL}};
    }
    return (struct se_node){.tag = NUMBER, .int_val = get_list_size(&element.list_val)};
}

struct se_node adds(struct se_list *list)
{
    int r = eval_object(list->element).int_val;
    list = &list->next->list_val;
    while (list != NULL)
    {
        r += eval_object(list->element).int_val;
        list = &list->next->list_val;
    }
    return (struct se_node){.tag = NUMBER, .int_val = r};
}

struct se_node subs(struct se_list *list)
{
    int r = eval_object(list->element).int_val;
    list = &list->next->list_val;
    while (list != NULL)
    {
        r -= eval_object(list->element).int_val;
        list = &list->next->list_val;
    }
    return (struct se_node){.tag = NUMBER, .int_val = r};
}

struct se_node muls(struct se_list *list)
{
    int r = eval_object(list->element).int_val;
    list = &list->next->list_val;
    while (list != NULL)
    {
        r *= eval_object(list->element).int_val;
        list = &list->next->list_val;
    }
    return (struct se_node){.tag = NUMBER, .int_val = r};
}

struct se_node divs(struct se_list *list)
{
    int r = eval_object(list->element).int_val;
    list = &list->next->list_val;
    while (list != NULL)
    {
        r /= eval_object(list->element).int_val;
        list = &list->next->list_val;
    }
    return (struct se_node){.tag = NUMBER, .int_val = r};
}

struct se_node matmul(struct se_list *list)
{
    struct se_node mat1 = eval_object(list->element);
    if (mat1.tag != MATRIX)
    {
        EV_LOG = "Matrix multiplication must be between two matrix objects!";
        return (struct se_node){.tag = LIST, .list_val = {.element = NULL, .next = NULL}};
    }
    struct se_node mat2 = eval_object(list->next->list_val.element);
    if (mat2.tag != MATRIX)
    {
        EV_LOG = "Matrix multiplication must be between two matrix objects!";
        return (struct se_node){.tag = LIST, .list_val = {.element = NULL, .next = NULL}};
    }
    if (mat1.matrix_val.columns != mat2.matrix_val.rows)
    {
        sprintf(mylog, "Matrix multiplication must be between two matrix objects of shapes"
                       "(M,N) and (N,P). Got (%d,%d) and (%d,%d)",
                mat1.matrix_val.rows, mat1.matrix_val.columns,
                mat2.matrix_val.rows, mat2.matrix_val.columns);
        EV_LOG = mylog;
        return (struct se_node){.tag = LIST, .list_val = {.element = NULL, .next = NULL}};
    }
    int newmatR = mat1.matrix_val.rows;
    int newmatC = mat2.matrix_val.columns;
    int *values = calloc(sizeof(int), newmatR * newmatC);
    for (int r = 0; r < newmatR; r++)
    {
        for (int c = 0; c < newmatC; c++)
        {
            for (int j = 0; j < mat1.matrix_val.columns; j++)
            {
                int value = eval_object(matrix_index_ptr(mat1.matrix_val, r, j)).int_val;
                value *= eval_object(matrix_index_ptr(mat2.matrix_val, j,c)).int_val;
                values[r * newmatC + c] += value;
            }
        }
    }
    struct se_node *list_node = calloc(sizeof(struct se_node),1);
    list_node->tag = LIST;
    for(int i = 0; i < newmatR * newmatC; i++) {
        struct se_node *number_node = calloc(sizeof(struct se_node),1);
        number_node->tag = NUMBER;
        number_node->int_val = values[i];
        add_to_list(list_node, number_node);
    }
    struct se_node matrix_node;
    matrix_node.tag = MATRIX;
    matrix_node.matrix_val.columns = newmatC;
    matrix_node.matrix_val.rows = newmatR;
    matrix_node.matrix_val.list = list_node;
    return matrix_node;
}

void init_map()
{
    funtable = (struct function_table){.entries = calloc(sizeof(struct table_entry), 4), .entry_count = 0, .entry_size = 4};
    strcpy(mylog, "adding add...");
    put_entry(&funtable, (struct se_symbol){.sym = "ADD\0\0\0\0", .sym_size = 3}, adds);
    sprintf(mylog + strlen(mylog), "fcount:%d", funtable.entry_count);
    strcat(mylog, "adding sub...");
    put_entry(&funtable, (struct se_symbol){.sym = "SUB\0\0\0\0", .sym_size = 3}, subs);
    sprintf(mylog + strlen(mylog), "fcount:%d", funtable.entry_count);
    strcat(mylog, "adding div...");
    put_entry(&funtable, (struct se_symbol){.sym = "DIV\0\0\0\0", .sym_size = 3}, divs);
    sprintf(mylog + strlen(mylog), "fcount:%d", funtable.entry_count);
    strcat(mylog, "adding mul...");
    put_entry(&funtable, (struct se_symbol){.sym = "MUL\0\0\0\0", .sym_size = 3}, muls);
    sprintf(mylog + strlen(mylog), "fcount:%d", funtable.entry_count);
    strcat(mylog, "adding lslen...");
    put_entry(&funtable, (struct se_symbol){.sym = "LSLEN\0\0", .sym_size = 5}, listsize);
    sprintf(mylog + strlen(mylog), "fcount:%d", funtable.entry_count);
    strcat(mylog, "adding lsget...");
    put_entry(&funtable, (struct se_symbol){.sym = "LSGET\0\0", .sym_size = 5}, listget);
    sprintf(mylog + strlen(mylog), "fcount:%d", funtable.entry_count);
    strcat(mylog, "adding asmat...");
    put_entry(&funtable, (struct se_symbol){.sym = "ASMAT\0\0", .sym_size = 5}, asmat);
    sprintf(mylog + strlen(mylog), "fcount:%d", funtable.entry_count);
    strcat(mylog, "adding matget...");
    put_entry(&funtable, (struct se_symbol){.sym = "MATGET\0", .sym_size = 6}, matget);
    sprintf(mylog + strlen(mylog), "fcount:%d", funtable.entry_count);
    strcat(mylog, "adding matmul...");
    put_entry(&funtable, (struct se_symbol){.sym = "MATMUL\0", .sym_size = 6}, matmul);
    sprintf(mylog + strlen(mylog), "fcount:%d", funtable.entry_count);
    strcat(mylog, "DONE!\r\n");
    EV_LOG = mylog;
}

void delete_object(struct se_node *rootw)
{
    if (rootw->tag == LIST)
    {
        struct se_node *el = rootw->list_val.element;
        struct se_node *nx = rootw->list_val.next;
        free(rootw);
        if (el != NULL)
            delete_object(el);
        if (nx != NULL)
            delete_object(nx);
    }
    else if (rootw->tag == MATRIX)
    {
        struct se_node *list = rootw->matrix_val.list;
        free(rootw);
        if (list != NULL)
            delete_object(list);
    }
    else
    {
        free(rootw);
    }
}

void log_object(struct se_node *rootw, char *logBuffer)
{
    switch (rootw->tag)
    {
    case NUMBER:
        sprintf(logBuffer + strlen(logBuffer), "%d", rootw->int_val);
        break;
    case SYMBOL:
    {
        char symname[7];
        memset(symname, 0, 7);
        memcpy(symname, rootw->sym_val.sym, rootw->sym_val.sym_size);
        symname[rootw->sym_val.sym_size] = 0;
        sprintf(logBuffer + strlen(logBuffer), "'%s", symname);
        break;
    }
    case MATRIX:
    {
        struct se_matrix matrix = rootw->matrix_val;
        sprintf(logBuffer + strlen(logBuffer), "\r\n[");
        for (int r = 0; r < matrix.rows; r++)
        {
            for (int c = 0; c < matrix.columns; c++)
            {
                log_object(matrix_index_ptr(matrix, r, c), logBuffer);
                sprintf(logBuffer + strlen(logBuffer), " ");
            }
            if (r != matrix.rows - 1)
                sprintf(logBuffer + strlen(logBuffer), "\r\n ");
        }
        sprintf(logBuffer + strlen(logBuffer), "]");
        break;
    }
    case LIST:
    {
        struct se_list *listNode = &rootw->list_val;
        sprintf(logBuffer + strlen(logBuffer), "(");
        log_object(listNode->element, logBuffer);
        while (listNode->next != NULL)
        {
            sprintf(logBuffer + strlen(logBuffer), " ");
            log_object(listNode->next->list_val.element, logBuffer);
            listNode = &listNode->next->list_val;
        }
        sprintf(logBuffer + strlen(logBuffer), ")");
        break;
    }
    }
}

struct se_node eval_object(struct se_node *node)
{
    switch (node->tag)
    {
    case LIST:
    {
        struct se_node *function = node->list_val.element;
        struct se_node result;
        if (function->tag != SYMBOL)
        {
            result = *node;
        }
        else
        {
            struct table_entry *func = get_entry(&funtable, function->sym_val);
            if (func != NULL)
            {
                result = func->function(&node->list_val.next->list_val);
            }
            else
            {
                strcat(mylog, "Failed to find function ");
                strcat(mylog, function->sym_val.sym);
                result = *node;
            }
        }
        return result;
    }
    default:
        return *node;
    }
}
