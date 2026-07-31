#include "pico/stdlib.h"
#include "hardware/uart.h"
#include "pico/multicore.h"
#include "ch9350l.h"
#include <stdio.h>
#include <string.h>
#include <machine/endian.h>

#include "ch9329.h"
#include "hardware/watchdog.h"

// CH9350L documentation: https://wiki.kamamilabs.com/images/b/bf/CH9350DS.pdf

// Pin where the CH9350 is plugged
#define UART1_TX_PIN 4
#define UART1_RX_PIN 5

// Pin where the CH9329 is plugged
#define UART0_TX_PIN 1
#define UART0_RX_PIN 0

int main() {
    stdio_init_all();

    /*
    watchdog_enable(5000, 1);
            watchdog_update(); // do it on both core
*/

    // FIXME: remove
    sleep_ms(3000);
    printf("KVM starting\n");

    ch9350l_t *hid_input = ch9350l_init_with_builtin_uart(uart1, UART1_TX_PIN, UART1_RX_PIN, 115200);
    ch9329_t *computer_1 = ch9329_init_with_builtin_uart(uart0, UART0_TX_PIN, UART0_RX_PIN, 115200);

    ch9329_send_get_info(computer_1);

    printf("KVM ready\n");

    uint8_t counter = 0;
    while (true) {
        ch9350l_message_t *message = ch9350l_dequeue_message(hid_input);
        if (message) {
            // When enabling debug, if too many message are printed, it will not be able to process messages fast enough and it will lose some messages
            // ch9350l_debug_print_message(message);
            if (message->type == CH9350_MESSAGE_TYPE_STATUS_REQUEST_FRAME) {
                ch9350l_set_to_state_1(hid_input);
            } else if (message->type == CH9350_MESSAGE_TYPE_VALID_KEY_VALUE_FRAME) {
                const ch9350l_valid_key_value_frame_t *msg_data
                        = (ch9350l_valid_key_value_frame_t *) message->data;
                printf("Key pressed (%s, %s %d): \n ",
                       ch9350l_debug_protocol_to_string(msg_data->labeling.protocol),
                       ch9350l_debug_kind_to_string(msg_data->labeling.kind),
                       msg_data->labeling.port
                );
                for (int i = 0; i < msg_data->key_value_data_length; i++) {
                    printf("%02x ", msg_data->key_value_data[i]);
                }
                printf("\n");

                /*
                ch9350l_set_keyboard_report(hid_input, counter++);
                if (counter >= 8) {
                    counter = 0;
                }
                */

                if (msg_data->labeling.kind == CH9350L_VALID_KEY_KIND_KEYBOARD) {
                    ch9329_send_keyboard_data(computer_1, msg_data->key_value_data, msg_data->key_value_data_length);
                } else if (msg_data->labeling.kind == CH9350L_VALID_KEY_KIND_MOUSE) {
                    // FIXME: Mouse descriptor is missing some parts
                }


                // FIXME: process key
            } else {
                ch9350l_debug_print_message(message);
            }
            ch9350l_message_free(hid_input, message);
        }

        ch9329_frame_t *frame = ch9329_dequeue_frame(computer_1);
        if (frame) {
            ch9329_debug_print_frame(frame);
            if  ((frame->command & 0xf) == CH9329_COMMAND_GET_INFO) {
                if (frame->data_length == sizeof(computer_1->para_cfg)) {
                    memcpy(&computer_1->para_cfg, frame->data, sizeof(computer_1->para_cfg));
                } else {
                    printf("Invalid para_cfg length: %d\n", frame->data_length);
                }
                ch9329_send_get_para_cfg(computer_1);
            }
            if  ((frame->command & 0xf) == CH9329_COMMAND_GET_PARA_CFG) {
                // ch9329_send_get_usb_string(computer_1, CH9329_USB_STRING_DESCRIPTOR_PRODUCT);
                /*computer_1->para_cfg.serial_port_communication_baud_rate = __bswap32(115200);
                ch9329_send_set_para_cfg(computer_1);*/
                /*ch9329_send_get_usb_string(computer_1, CH9329_USB_STRING_DESCRIPTOR_MANUFACTURER);
                ch9329_send_get_usb_string(computer_1, CH9329_USB_STRING_DESCRIPTOR_MANUFACTURER);*/
            }
            ch9329_frame_free(computer_1, frame);
        }
    }
}
