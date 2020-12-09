#include "drv_usb_hw.h"
#include "cdc_acm_core.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "lcd/lcd.h"
#include "gd32v_pjt_include.h"
#include "se_header.h"
#include "tok_header.h"
#include "ev_header.h"

extern uint8_t packet_sent, packet_receive;
extern uint32_t receive_length;
extern char *EV_LOG;
extern struct function_table funtable;

int bytes_received = 0;
int bytes_sent = 0;

usb_core_driver USB_OTG_dev =
    {
        .dev = {
            .desc = {
                .dev_desc = (uint8_t *)&device_descriptor,
                .config_desc = (uint8_t *)&configuration_descriptor,
                .strings = usbd_strings,
            }}};

void cdc_print(usb_dev *pudev, uint8_t *str, uint32_t data_len);

void usb_init()
{
    eclic_global_interrupt_enable();

    eclic_priority_group_set(ECLIC_PRIGROUP_LEVEL2_PRIO2);

    usb_rcu_config();

    usb_timer_init();

    usb_intr_config();

    usbd_init(&USB_OTG_dev, USB_CORE_ENUM_FS, &usbd_cdc_cb);

    /* check if USB device is enumerated successfully */
    while (USBD_CONFIGURED != USB_OTG_dev.dev.cur_status)
    {
    }
}

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
    cdc_print(&USB_OTG_dev, log, strlen(log));
}

int main(void)
{
    int idx = 0;
    uint8_t rcvstr[64] = "";
    uint8_t command_buffer[2048] = "";
    // Initialize LEDs
    rcu_periph_clock_enable(RCU_GPIOA);
    rcu_periph_clock_enable(RCU_GPIOC);
    gpio_init(GPIOC, GPIO_MODE_OUT_PP, GPIO_OSPEED_50MHZ, GPIO_PIN_13);
    gpio_init(GPIOA, GPIO_MODE_OUT_PP, GPIO_OSPEED_50MHZ, GPIO_PIN_1 | GPIO_PIN_2);
    LEDR(0);
    LEDG(1);
    LEDB(1);

    usb_init();
    init_map();
    memset(command_buffer, 0, sizeof(command_buffer));

    while (1)
    {
        LEDR(0);
        LEDG(0);
        LEDB(0);
        if (USBD_CONFIGURED == USB_OTG_dev.dev.cur_status)
        {
            if (1 == packet_receive && 1 == packet_sent)
            {
                packet_sent = 0;
                /* receive data from the host when the last packet data have been sent to the host */
                //cdc_acm_data_receive(&USB_OTG_dev);
                usbd_ep_recev(&USB_OTG_dev, CDC_ACM_DATA_OUT_EP, rcvstr, 64);
            }
            else
            {
                if (0 != receive_length)
                {
                    bytes_received += receive_length;
                    for (int i = 0; i < receive_length; i++)
                    {
                        LEDR(1);
                        LEDG(1);
                        LEDB(1);
                        if ((rcvstr[i] == ';'))
                        {
                            command_buffer[idx] = 0;

                            parse_command(command_buffer, idx);
                            idx = 0;
                            memset(command_buffer, 0, sizeof(command_buffer));
                            LEDR(1);
                            LEDG(0);
                            LEDB(0);
                        }
                        else if (rcvstr[i] == '?')
                        {
                            char *out_msg = EV_LOG == NULL ? "No message\r\n" : EV_LOG;
                            cdc_print(&USB_OTG_dev, out_msg, strlen(out_msg));
                            LEDR(1);
                            LEDG(1);
                            LEDB(1);
                        }
                        else if (rcvstr[i] == '$')
                        {
                            char known_functions[256] = "function count:";
                            sprintf(known_functions + strlen(known_functions), "%d\r\n", funtable.entry_count);
                            for (int i = 0; i < funtable.entry_count; i++)
                            {
                                sprintf(known_functions + strlen(known_functions), "%lu -> %p;", funtable.entries[i].key, funtable.entries[i].function);
                            }
                            sprintf(known_functions + strlen(known_functions), "\r\n");
                            cdc_print(&USB_OTG_dev, known_functions, strlen(known_functions));
                            LEDR(1);
                            LEDG(1);
                            LEDB(1);
                        }
                        else
                        {
                            // Copy received data to buffer
                            if (idx < sizeof(command_buffer) - 1)
                            {
                                command_buffer[idx] = rcvstr[i];
                                command_buffer[idx + 1] = '\r';
                                idx++;
                                // echo received characters
                                cdc_print(&USB_OTG_dev, command_buffer, idx + 2);
                            }
                            LEDR(0);
                            LEDG(1);
                            LEDB(1);
                        }
                    }
                    receive_length = 0;
                }
            }
        }
        else
        {
            /* wait until USB device is enumerated again */
            while (USBD_CONFIGURED != USB_OTG_dev.dev.cur_status)
            {
            }
        }
    }
}

// Write to USB CDC, reading should be similar
void cdc_print(usb_dev *pudev, uint8_t *str, uint32_t data_len)
{
    packet_sent = 0;
    usbd_ep_send(pudev, CDC_ACM_DATA_IN_EP, str, data_len);
    bytes_sent += data_len;
}