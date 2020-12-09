#ifndef SE_HEADER
#define SE_HEADER
#include <stdlib.h>
#include <stdint.h>
#include "tok_header.h"

struct se_node;
struct se_list
{
    struct se_node *element;
    struct se_node *next;
};

struct se_symbol
{
    char sym[7];
    uint8_t sym_size;
};

struct se_node
{
    union
    {
        int int_val;
        struct se_symbol sym_val;
        struct se_list list_val;
    };
    enum node_type
    {
        NUMBER,
        SYMBOL,
        LIST
    } tag;
};
extern struct se_node *add_to_list(struct se_node *list_node, struct se_node *element);
extern struct se_node *parse_tokens(struct token *start_token, uint8_t *token_count);

#endif