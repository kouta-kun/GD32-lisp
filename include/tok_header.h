#ifndef TOK_HEADER
#define TOK_HEADER
#include <stdint.h>

struct token
{
    char *TEXT;
    uint8_t token_size;
    enum tok_type
    {
        LPAREN,
        RPAREN,
        ADD,
        SUB,
        MUL,
        DIV,
        NUM,
        SYM
    } tag;
};
extern void tokenize_command(uint8_t *command, uint32_t string_len, struct token tokens[64], uint8_t *token_count);
#endif