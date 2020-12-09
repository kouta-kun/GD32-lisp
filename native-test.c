#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "se_header.h"
#include "tok_header.h"
#include "ev_header.h"

extern uint8_t packet_sent, packet_receive;
extern uint32_t receive_length;
extern char *EV_LOG;
extern struct function_table funtable;

void parse_command(uint8_t *command, uint32_t string_len)
{
    char log[1024] = "";
    struct token tokens[64];
    uint8_t token_count = 0;

    tokenize_command(command, string_len, tokens, &token_count);

    struct se_node *current_node = parse_tokens(tokens, &token_count);
    sprintf(log, "\r\ntoken_count=%d\r\n", token_count);
    sprintf(log + strlen(log), "value of evaluating ");
    log_object(current_node, log);
    sprintf(log + strlen(log), " is ");
    struct se_node result = eval_object(current_node);
    log_object(&result, log);
    sprintf(log + strlen(log), "\r\n\n");
    puts(log);
}

int main(void)
{
    int idx = 0;
    uint8_t command_buffer[2048] = "";
    // Initialize LEDs
    init_map();
    memset(command_buffer, 0, sizeof(command_buffer));

    while (1) {
      fgets(command_buffer, 2047, stdin);
      parse_command(command_buffer, strlen(command_buffer));
      memset(command_buffer, 0, sizeof(command_buffer));
    }

}
