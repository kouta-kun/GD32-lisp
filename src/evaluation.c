#include "ev_header.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

struct function_table funtable;

char *EV_LOG = NULL;
char mylog[512];

static struct map_entry *get_entry(struct function_table *table, struct se_symbol symbol)
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

    struct map_entry *node = table->entries;
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

static int create_entry(struct function_table *table, struct map_entry **reference_entry)
{
    int entryIndex = table->entry_count++;
    if (table->entry_count > table->entry_size)
    {
        int oldSize = table->entry_size;
        table->entry_size *= 2;
        struct map_entry *newEntries = calloc(sizeof(struct map_entry), (table->entry_size));
        struct map_entry *oldEntries = table->entries;
        memcpy(newEntries, oldEntries, sizeof(struct map_entry) * (oldSize));
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

static struct map_entry *put_entry(struct function_table *table, struct se_symbol symbol, se_function function)
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
        table->entries[0] = (struct map_entry){.key = key};
        table->entry_count++;
    }

    sprintf(mylog + strlen(mylog), "(hash for %s:%lu)", symname, key);

    struct map_entry *destNode = table->entries;
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

struct se_node listget(struct se_list *list)
{
    int list_index = list->next->list_val.element->int_val;
    list = &list->element->list_val;
    while (list_index-- > 0)
    {
        list = &list->next->list_val;
    }
    return *list->element;
}

struct se_node listsize(struct se_list *list)
{
    list = &list->element->list_val;
    int list_size = 0;
    while (list != NULL)
    {
        list_size++;
        list = &list->next->list_val;
    }
    return (struct se_node){.tag = NUMBER, .int_val = list_size};
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

void init_map()
{
    funtable = (struct function_table){.entries = calloc(sizeof(struct map_entry), 4), .entry_count = 0, .entry_size = 4};
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
    strcat(mylog, "DONE!\r\n");
    EV_LOG = mylog;
}

void delete_object(struct se_node *rootw)
{
    if (rootw->tag != LIST)
    {
        free(rootw);
    }
    else
    {
        struct se_node *el = rootw->list_val.element;
        struct se_node *nx = rootw->list_val.next;
        free(rootw);
        if (el != NULL)
            delete_object(el);
        if (nx != NULL)
            delete_object(nx);
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
    case LIST:
        sprintf(logBuffer + strlen(logBuffer), "(");
        struct se_list *listNode = &rootw->list_val;
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
            struct map_entry *func = get_entry(&funtable, function->sym_val);
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
