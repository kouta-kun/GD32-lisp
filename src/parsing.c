#include "se_header.h"
#include <stdlib.h>
#include <string.h>

struct se_node *add_to_list(struct se_node *list_node, struct se_node *element)
{
    if (list_node->tag != LIST)
    {
        return NULL;
    }
    if (list_node->list_val.element == NULL)
    {
        list_node->list_val.element = element;
        return list_node;
    }
    else if (list_node->list_val.next == NULL)
    {
        list_node->list_val.next = calloc(sizeof(struct se_node), 1);
        *list_node->list_val.next = (struct se_node){.tag = LIST, .list_val = (struct se_list){.element = element, .next = NULL}};
        return list_node->list_val.next;
    }
    else
        return add_to_list(list_node->list_val.next, element);
}

struct se_node *parse_tokens(struct token *start_token, uint8_t *token_count)
{
    uint8_t tok_count = *token_count;
    struct se_node *returned_node;
    returned_node = calloc(sizeof(struct se_node), 1);
    switch (start_token->tag)
    {
    case LPAREN:
    {
        *returned_node = (struct se_node){.tag = LIST, .list_val = {NULL, NULL}};
        int list_tok_count;
        for (list_tok_count = 1; list_tok_count < tok_count; list_tok_count++)
        {
            if (start_token[list_tok_count].tag == RPAREN)
            {
                break;
            }
            uint8_t node_tokens = tok_count;
            struct se_node *list_item = parse_tokens(start_token + list_tok_count, &node_tokens);

            add_to_list(returned_node, list_item);
            list_tok_count += (node_tokens - 1);
        }
        *token_count = list_tok_count + 1;
        break;
    }
    case NUM:
    {
        char token_number[start_token->token_size + 1];
        memcpy(token_number, start_token->TEXT, start_token->token_size);
        token_number[start_token->token_size] = 0;
        *returned_node = (struct se_node){.tag = NUMBER, .int_val = atoi(token_number)};
        *token_count = 1;
        break;
    }
    case SYM:
    {
        *returned_node = (struct se_node){
            .tag = SYMBOL,
            .sym_val = (struct se_symbol){.sym_size = start_token->token_size}};
        memset(returned_node->sym_val.sym, 0, 7);
        memcpy(returned_node->sym_val.sym, start_token->TEXT, start_token->token_size);
        *token_count = 1;
        break;
    }
    case ADD:
    {
        *returned_node = (struct se_node){.tag = SYMBOL, .sym_val = (struct se_symbol){.sym = "ADD", .sym_size = 3}};
        *token_count = 1;
        break;
    }
    case SUB:
    {
        *returned_node = (struct se_node){.tag = SYMBOL, .sym_val = (struct se_symbol){.sym = "SUB", .sym_size = 3}};
        *token_count = 1;
        break;
    }
    case MUL:
    {
        *returned_node = (struct se_node){.tag = SYMBOL, .sym_val = (struct se_symbol){.sym = "MUL", .sym_size = 3}};
        *token_count = 1;
        break;
    }
    case DIV:
    {
        *returned_node = (struct se_node){.tag = SYMBOL, .sym_val = (struct se_symbol){.sym = "DIV", .sym_size = 3}};
        *token_count = 1;
        break;
    }
    }
    return returned_node;
}