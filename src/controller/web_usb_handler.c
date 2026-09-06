#include "web_usb_handler.h"

#include "computer_manager.h"
#include "hid_device_manager.h"
#include "hid_manager.h"
#include "logger.h"
#include "utils.h"
#include "common/tusb_types.h"
#include "device/usbd.h"

// Out / In are relative to the host
static uint8_t vendor_data_out_buffer[64];
static uint8_t vendor_data_in_buffer[2048];

static bool write_to_buffer(
    uint8_t *buffer,
    const size_t buffer_size,
    size_t *offset,
    const void *data,
    const size_t data_len
) {
    if (*offset + data_len > buffer_size) {
        return false;
    }
    memcpy(buffer + *offset, data, data_len);
    *offset = *offset + data_len;
    return true;
}

#define write_value_to_buffer(buffer, buffer_size, offset, data) \
    write_to_buffer(buffer, buffer_size, offset, &data, sizeof(data))

bool tud_vendor_control_xfer_cb(
    const uint8_t rhport,
    const uint8_t stage,
    tusb_control_request_t const *request
) {
    if (request->bmRequestType_bit.type != TUSB_REQ_TYPE_VENDOR)
        return false;

    const bool is_in = request->bmRequestType_bit.direction == TUSB_DIR_IN;
    if (IS_IN_COMMAND(request->bRequest) != is_in) {
        logf_error("Invalid direction for vendor command: %u", request->bRequest);
        return false;
    }

    switch (stage) {
        case CONTROL_STAGE_SETUP: {
            if (IS_IN_COMMAND(request->bRequest)) {
                switch (request->bRequest) {
                    case COMMAND_IN_OP_GET_INFO: {
                        auto const data = (web_usb_cmd_get_info_command_data_t *) vendor_data_in_buffer;
                        data->version = 1; // FIXME: String ?
                        data->protocol_version = WEB_USB_PROTOCOL_VERSION;
                        data->computer_count = MAX_COMPUTER;
                        data->hid_interface_count = CFG_TUD_HID;
                        data->hid_device_count = MAX_HID_DEVICE;
                        return tud_control_xfer(rhport, request, data, sizeof(*data));
                    }
                    case COMMAND_IN_OP_GET_COMPUTER_STATE: {
                        auto const data = (web_usb_cmd_get_computer_state_data_t *) vendor_data_in_buffer;
                        const computer_t *computer = computer_manager_get_computer(request->wValue);
                        if (computer == nullptr) {
                            return false;
                        }
                        data->computer_id = computer->computer_id;
                        data->state = computer->state;
                        memcpy(data->hid_protocol_per_interface, computer->hid_protocol_per_interface,
                               CFG_TUD_HID * sizeof(uint8_t));
                        return tud_control_xfer(rhport, request, data, sizeof(*data));
                    }
                    case COMMAND_IN_OP_GET_HID_DEVICE: {
                        auto const data = (web_usb_cmd_get_hid_device_data_t *) vendor_data_in_buffer;
                        const hid_device_t *hid_device = hid_device_manager_get(request->wValue);
                        if (hid_device == nullptr) {
                            memset(data, 0, sizeof(*data));
                        return tud_control_xfer(rhport, request, data, sizeof(*data));
                        } else {
                            data->dev_addr = hid_device->dev_addr;
                            data->is_mounted = hid_device->is_mounted;

                            const size_t buffer_size = min16(sizeof(vendor_data_in_buffer), request->wLength);
                            size_t offset = sizeof(web_usb_cmd_get_hid_device_data_t);
                            uint8_t *buffer = vendor_data_in_buffer;
                            write_value_to_buffer(buffer, buffer_size, &offset, hid_device->manufacturer_name_len);
                            write_to_buffer(buffer, buffer_size, &offset, hid_device->manufacturer_name, hid_device->manufacturer_name_len);
                            write_value_to_buffer(buffer, buffer_size, &offset, hid_device->product_name_len);
                            write_to_buffer(buffer, buffer_size, &offset, hid_device->product_name, hid_device->product_name_len);
                            return tud_control_xfer(rhport, request, data, offset);
                        }
                    }
                    case COMMAND_IN_OP_GET_HID_STATE: {
                        auto const data = (web_usb_cmd_get_hid_state_data_t *) vendor_data_in_buffer;
                        const hid_t *hid = hid_mgr_get_by_kvm_idx(request->wValue);
                        if (hid == nullptr) {
                            memset(data, 0, sizeof(*data));
                        } else {
                            data->enabled = hid->enabled;
                            data->dev_addr = hid->dev_addr;
                            data->host_hid_idx = hid->host_hid_idx;
                            data->kvm_hid_idx = hid->kvm_hid_idx;
                            data->itf_protocol = hid->itf_protocol;
                            data->use_report_id = hid->use_report_id;
                            data->has_keyboard_report = hid->has_keyboard_report;
                            data->vid = hid->vid;
                            data->pid = hid->pid;
                        }
                        return tud_control_xfer(rhport, request, data, sizeof(*data));
                    }
                    case COMMAND_IN_OP_GET_HID_DESCRIPTOR: {
                        const hid_t *hid = hid_mgr_get_by_kvm_idx(request->wValue);
                        if (hid == nullptr)
                            return false;

                        return tud_control_xfer(rhport, request, hid->raw_report_descriptor,
                                                hid->raw_report_descriptor_len);
                    }
                    case COMMAND_IN_OP_GET_GENERAL_CONFIG: {
                        auto const data = (web_usb_cmd_get_general_config_data_t *) vendor_data_in_buffer;
                        const general_config_t *config = kvm_config_get_general();
                        data->vid = config->vid;
                        data->pid = config->pid;
                        return tud_control_xfer(rhport, request, data, sizeof(*data));
                    }
                    case COMMAND_IN_OP_GET_KEYBOARD_SHORTCUTS: {
                        auto const data = (web_usb_cmd_get_keyboard_shortcuts_command_data_t *) vendor_data_in_buffer;
                        data->keyboard_shortcut_count = MAX_KEYBOARD_SHORTCUT;
                        return tud_control_xfer(rhport, request, data, sizeof(*data));
                    }
                    case COMMAND_IN_OP_GET_KEYBOARD_SHORTCUT: {
                        auto const data = (web_usb_cmd_get_keyboard_shortcut_data_t *) vendor_data_in_buffer;

                        const keyboard_shortcut_t *keyboard_shortcut = kvm_config_get_shortcut(request->wValue);
                        if (keyboard_shortcut == nullptr) {
                            return false;
                        }

                        data->shortcut_id = keyboard_shortcut->shortcut_id;
                        data->action = keyboard_shortcut->action;
                        data->enabled = keyboard_shortcut->enabled;
                        data->key_count = keyboard_shortcut->key_count;
                        memcpy(data->keys, keyboard_shortcut->keys,
                               keyboard_shortcut->key_count * sizeof(keyboard_shortcut->keys[0]));
                        data->data_len = keyboard_shortcut->data_len;
                        memcpy(data->data, keyboard_shortcut->data, keyboard_shortcut->data_len);

                        return tud_control_xfer(rhport, request, data, sizeof(*data));
                    }
                    case COMMAND_IN_OP_GET_LOGS: {
                        size_t offset = 0;
                        uint8_t *buffer = vendor_data_in_buffer;
                        const size_t buffer_size = min16(sizeof(vendor_data_in_buffer), request->wLength);

                        log_t log;
                        while (try_dequeue_log(&log)) {
                            const size_t last_offset = offset;
                            bool success = true;
                            success = success && write_value_to_buffer(buffer, buffer_size, &offset, log.timestamp);
                            success = success && write_value_to_buffer(buffer, buffer_size, &offset, log.log_level);
                            success = success && write_value_to_buffer(buffer, buffer_size, &offset, log.line);
                            success = success && write_value_to_buffer(buffer, buffer_size, &offset, log.func_len);
                            success = success && write_to_buffer(buffer, buffer_size, &offset, log.func, log.func_len);
                            success = success && write_value_to_buffer(buffer, buffer_size, &offset, log.msg_len);
                            success = success && write_to_buffer(buffer, buffer_size, &offset, log.msg, log.msg_len);
                            if (!success) {
                                // Last log is lost if this happen (it should not) if this happen we'll fix this
                                logf_error("Failed to write log message to buffer: buffer_size=%u, offset=%u",
                                           buffer_size, offset);
                                offset = last_offset;
                                break;
                            }
                            if (buffer_size - offset <= sizeof(log_t))
                                break;
                        }

                        return tud_control_xfer(rhport, request, buffer, offset);
                    }
                    default: {
                        break;
                    }
                }
                return tud_control_xfer(rhport, request, nullptr, 0);
            }

            if (IS_OUT_COMMAND(request->bRequest)) {
                if (request->wLength > sizeof(vendor_data_out_buffer)) {
                    log_error("Data size sent to vendor interface is too large");
                    return false;
                }
                memset(vendor_data_out_buffer, 0, sizeof(vendor_data_out_buffer));
                return tud_control_xfer(rhport, request, vendor_data_out_buffer, request->wLength);
            }
            break;
        }
        case CONTROL_STAGE_DATA: {
            if (IS_IN_COMMAND(request->bRequest)) {
                return true;
            }

            if (IS_OUT_COMMAND(request->bRequest)) {
                switch (request->bRequest) {
                    case COMMAND_OUT_OP_SET_GENERAL_CONFIG: {
                        auto const data = (web_usb_cmd_set_general_config_data_t *) vendor_data_out_buffer;
                        general_config_t *config = kvm_config_get_general();
                        config->vid = data->vid;
                        config->pid = data->pid;
                        return tud_control_xfer(rhport, request, nullptr, 0);
                    }
                    case COMMAND_OUT_OP_SET_SHORTCUT: {
                        auto const data = (web_usb_cmd_set_keyboard_shortcut_data_t *) vendor_data_out_buffer;
                        kvm_config_set_shortcut(
                            data->shortcut_id,
                            data->enabled,
                            data->action,
                            data->key_count,
                            data->keys,
                            data->data_len,
                            data->data
                        );
                        return tud_control_xfer(rhport, request, nullptr, 0);
                    }
                    default: {
                        break;
                    }
                }
            }
            break;
        }
        case CONTROL_STAGE_ACK: {
            if (IS_IN_COMMAND(request->bRequest)) {
                memset(vendor_data_in_buffer, 0, sizeof(vendor_data_in_buffer));
            }
            if (IS_OUT_COMMAND(request->bRequest)) {
                memset(vendor_data_out_buffer, 0, sizeof(vendor_data_out_buffer));
            }
            return true;
        }
        default: {
            break;
        }
    }
    return false;
}
