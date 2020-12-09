#include "tok_header.h"

void tokenize_command(uint8_t *command, uint32_t string_len, struct token tokens[64], uint8_t *token_count)
{
    int toksize;
    for (int i = 0; i < string_len; i++)
    {
        switch (command[i])
        {
        case '(':
            tokens[(*token_count)++] = (struct token){.TEXT = command + i, .tag = LPAREN, .token_size = 1};
            break;
        case ')':
            tokens[(*token_count)++] = (struct token){.TEXT = command + i, .tag = RPAREN, .token_size = 1};
            break;
        case '+':
            tokens[(*token_count)++] = (struct token){.TEXT = command + i, .tag = ADD, .token_size = 1};
            break;
        case '-':
            tokens[(*token_count)++] = (struct token){.TEXT = command + i, .tag = SUB, .token_size = 1};
            break;
        case '*':
            tokens[(*token_count)++] = (struct token){.TEXT = command + i, .tag = MUL, .token_size = 1};
            break;
        case '/':
            tokens[(*token_count)++] = (struct token){.TEXT = command + i, .tag = DIV, .token_size = 1};
            break;
        case '\'':
            i++;
            for (toksize = 0; toksize < string_len - i; toksize++)
            {
                if (!(command[i + toksize] >= 'A' && command[i + toksize] <= 'Z'))
                {
                    tokens[(*token_count)++] = (struct token){.TEXT = command + i, .tag = SYM, .token_size = toksize};
                    i = i + toksize - 1;
                    break;
                }
            }
            break;
        case '0':
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9':
            for (toksize = 0; toksize < string_len - i; toksize++)
            {
                if (!(command[i + toksize] >= '0' && command[i + toksize] <= '9'))
                {
                    tokens[(*token_count)++] = (struct token){.TEXT = command + i, .tag = NUM, .token_size = toksize};
                    i = i + toksize - 1;
                    break;
                }
            }
            break;
        default:
            break;
        }
    }
}