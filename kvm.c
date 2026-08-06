#include "pico/stdlib.h"
#include "hardware/uart.h"
#include "pico/multicore.h"
#include "ch9350l.h"
#include <stdio.h>
#include <string.h>

#include "debug.h"
#include "internal_com.h"
#include "usb_device_descriptor.h"
#include "bsp/board_api.h"
#include "hardware/clocks.h"
#include "hardware/watchdog.h"
#include "pico/bootrom.h"
#include "pico/util/queue.h"

// CH9350L documentation: https://wiki.kamamilabs.com/images/b/bf/CH9350DS.pdf

// Pin where the CH9350 is plugged
#define UART1_TX_PIN 4
#define UART1_RX_PIN 5

// Pin where the CH9329 is plugged
#define UART0_TX_PIN 1
#define UART0_RX_PIN 0

static queue_t htc_msg_queue = {};
static queue_t cth_msg_queue = {};

static inline int16_t sign_extend_12(uint16_t value) {
    if (value & 0x0800) {
        value |= 0xf000;
    }
    return (int16_t) value;
}

void core1_main() {
    printf("KVM starting\n");

    ch9350l_t *hid_input = ch9350l_init_with_builtin_uart(uart1, UART1_TX_PIN, UART1_RX_PIN, 115200);
    // ch9329_t *computer_1 = ch9329_init_with_builtin_uart(uart0, UART0_TX_PIN, UART0_RX_PIN, 115200);

    // ch9329_send_get_info(computer_1);

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
                /*
                printf("Key pressed (%s, %s %d): \n ",
                       ch9350l_debug_protocol_to_string(msg_data->labeling.protocol),
                       ch9350l_debug_kind_to_string(msg_data->labeling.kind),
                       msg_data->labeling.port
                );
                for (int i = 0; i < msg_data->key_value_data_length; i++) {
                    printf("%02x ", msg_data->key_value_data[i]);
                }
                printf("\n");
                */

                /*
                ch9350l_set_keyboard_report(hid_input, counter++);
                if (counter >= 8) {
                    counter = 0;
                }
                */

                if (msg_data->labeling.kind == CH9350L_VALID_KEY_KIND_KEYBOARD) {
                    hid_to_computer_message_t htc_message = {
                        .opcode = HID_TO_COMPUTER_KEYBOARD_MESSAGE,
                        .data_len = msg_data->key_value_data_length,
                    };
                    memcpy(&htc_message.data, msg_data->key_value_data, msg_data->key_value_data_length);
                    queue_try_add(&htc_msg_queue, &htc_message);
                } else if (msg_data->labeling.kind == CH9350L_VALID_KEY_KIND_MOUSE) {
                    hid_to_computer_message_t htc_message = {
                        .opcode = HID_TO_COMPUTER_MOUSE_MESSAGE,
                        .data_len = sizeof(htc_message_mouse_data_t),
                    };
                    htc_message_mouse_data_t *data = (htc_message_mouse_data_t *) htc_message.data;
                    data->buttons = msg_data->key_value_data[1];
                    uint16_t x = ((msg_data->key_value_data[4] & 0x0f) << 8) | msg_data->key_value_data[3];
                    uint16_t y = (msg_data->key_value_data[5] << 4) | ((msg_data->key_value_data[4] >> 4) & 0xf);
                    data->x = sign_extend_12(x);
                    data->y = sign_extend_12(y);
                    data->wheel = (int8_t) msg_data->key_value_data[6];
                    data->pan = (int8_t) msg_data->key_value_data[7];
                    queue_try_add(&htc_msg_queue, &htc_message);
                }


                // FIXME: process key
            } else {
                ch9350l_debug_print_message(message);
            }
            ch9350l_message_free(hid_input, message);
        }

        /*
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
                ch9329_send_set_para_cfg(computer_1);#1#
                /*ch9329_send_get_usb_string(computer_1, CH9329_USB_STRING_DESCRIPTOR_MANUFACTURER);
                ch9329_send_get_usb_string(computer_1, CH9329_USB_STRING_DESCRIPTOR_MANUFACTURER);#1#
            }
            ch9329_frame_free(computer_1, frame);
        }*/
        watchdog_update();
    }
}

int main() {
    stdio_init_all();
    watchdog_enable(5000, 1);

    queue_init(&htc_msg_queue, sizeof(hid_to_computer_message_t), 32);
    queue_init(&cth_msg_queue, sizeof(computer_to_hid_message_t), 32);

    multicore_launch_core1(core1_main);

    board_init();
    // init device stack on configured roothub port
    const tusb_rhport_init_t rh_init = {
        .role = TUSB_ROLE_DEVICE,
        .speed = TUD_OPT_HIGH_SPEED ? TUSB_SPEED_HIGH : TUSB_SPEED_FULL
    };
    TU_ASSERT(tud_rhport_init(BOARD_TUD_RHPORT, &rh_init));
    board_init_after_tusb();

    while (true) {
        tud_task(); // tinyusb device task


        if (!tud_hid_ready()) {
            continue;
        }
        hid_to_computer_message_t htc_message;
        if (queue_try_remove(&htc_msg_queue, &htc_message)) {
            switch (htc_message.opcode) {
                case HID_TO_COMPUTER_KEYBOARD_MESSAGE: {
                    if (htc_message.data[2] == 0x48) {
                        printf("Resetting USB\n");
                        multicore_reset_core1();
                        reset_usb_boot(0, 0);
                    }

                    tud_hid_keyboard_report(USB_DEVICE_REPORT_ID_KEYBOARD, htc_message.data[0], htc_message.data + 2);
                    break;
                }
                case HID_TO_COMPUTER_MOUSE_MESSAGE: {
                    htc_message_mouse_data_t *data = (htc_message_mouse_data_t *) htc_message.data;
                    tud_hid_mouse_report(USB_DEVICE_REPORT_ID_MOUSE,
                                             data->buttons,
                                             (int8_t)data->x,
                                             (int8_t)data->y,
                                             data->wheel,
                                             data->pan);
                    break;
                }
                default:
                    // FIXME: error
                    break;
            }
        }
        watchdog_update();
    }
}

uint16_t tud_hid_get_report_cb(uint8_t instance, uint8_t report_id, hid_report_type_t report_type, uint8_t *buffer,
                               uint16_t reqlen) {
    // TODO not Implemented
    (void) instance;
    (void) report_id;
    (void) report_type;
    (void) buffer;
    (void) reqlen;

    return 0;
}

void tud_hid_set_report_cb(uint8_t instance, uint8_t report_id, hid_report_type_t report_type, uint8_t const *buffer,
                           uint16_t bufsize) {
}

/*
static void send_hid_report(uint8_t report_id, uint32_t btn)
{
  // skip if hid is not ready yet
  if ( !tud_hid_ready() ) return;

  switch(report_id)
  {
    case REPORT_ID_KEYBOARD:
    {
      // use to avoid send multiple consecutive zero report for keyboard
      static bool has_keyboard_key = false;

      if ( btn )
      {
        uint8_t keycode[6] = { 0 };
        keycode[0] = HID_KEY_A;

        tud_hid_keyboard_report(REPORT_ID_KEYBOARD, 0, keycode);
        has_keyboard_key = true;
      }else
      {
        // send empty key report if previously has key pressed
        if (has_keyboard_key) tud_hid_keyboard_report(REPORT_ID_KEYBOARD, 0, NULL);
        has_keyboard_key = false;
      }
    }
    break;

    case REPORT_ID_MOUSE:
    {
      int8_t const delta = 5;

      // no button, right + down, no scroll, no pan
      tud_hid_mouse_report(REPORT_ID_MOUSE, 0x00, delta, delta, 0, 0);
    }
    break;

    case REPORT_ID_CONSUMER_CONTROL:
    {
      // use to avoid send multiple consecutive zero report
      static bool has_consumer_key = false;

      if ( btn )
      {
        // volume down
        uint16_t volume_down = HID_USAGE_CONSUMER_VOLUME_DECREMENT;
        tud_hid_report(REPORT_ID_CONSUMER_CONTROL, &volume_down, 2);
        has_consumer_key = true;
      }else
      {
        // send empty key report (release key) if previously has key pressed
        uint16_t empty_key = 0;
        if (has_consumer_key) tud_hid_report(REPORT_ID_CONSUMER_CONTROL, &empty_key, 2);
        has_consumer_key = false;
      }
    }
    break;

    case REPORT_ID_GAMEPAD:
    {
      // use to avoid send multiple consecutive zero report for keyboard
      static bool has_gamepad_key = false;

      hid_gamepad_report_t report =
      {
        .x   = 0, .y = 0, .z = 0, .rz = 0, .rx = 0, .ry = 0,
        .hat = 0, .buttons = 0
      };

      if ( btn )
      {
        report.hat = GAMEPAD_HAT_UP;
        report.buttons = GAMEPAD_BUTTON_A;
        tud_hid_report(REPORT_ID_GAMEPAD, &report, sizeof(report));

        has_gamepad_key = true;
      }else
      {
        report.hat = GAMEPAD_HAT_CENTERED;
        report.buttons = 0;
        if (has_gamepad_key) tud_hid_report(REPORT_ID_GAMEPAD, &report, sizeof(report));
        has_gamepad_key = false;
      }
    }
    break;

    default: break;
  }
}*/
