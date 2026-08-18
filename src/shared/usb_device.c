#include "usb_device.h"

#include "class/hid/hid.h"
#include "class/hid/hid_device.h"
#include "device/usbd.h"
#include "bsp/board_api.h"

#include "logger.h"
#include "hid_manager.h"

typedef struct {
    uint8_t computer_id;
    uint16_t vid;
    uint16_t pid;
    set_report_cb_t set_computer_report_cb;
    set_hid_protocol_cb_t set_computer_hid_protocol_cb;
} usb_device_t;

static usb_device_t usb_device;

// ╔══════════════════════════════════╗
// ║                Core              ║
// ╚══════════════════════════════════╝

void usb_device_init(
    const uint8_t computer_id,
    const uint16_t vid,
    const uint16_t pid,
    const set_report_cb_t set_report_cb,
    const set_hid_protocol_cb_t set_hid_protocol_cb
) {
    usb_device.computer_id = computer_id;
    usb_device.vid = vid;
    usb_device.pid = pid;
    usb_device.set_computer_report_cb = set_report_cb;
    usb_device.set_computer_hid_protocol_cb = set_hid_protocol_cb;
}

void usb_device_task() {
    tud_task(); // tinyusb device task
}

void usb_device_connect_to_computer(
    const uint8_t rhport
) {
    if (tud_inited()) {
        log_info("Re-initializing USB device");
        tud_deinit(rhport);
    } else {
        log_info("Initializing USB device");
    }

    // init device stack on configured roothub port
    const tusb_rhport_init_t rh_init = {
        .role = TUSB_ROLE_DEVICE,
        .speed = TUD_OPT_HIGH_SPEED ? TUSB_SPEED_HIGH : TUSB_SPEED_FULL
    };
    if (!tud_rhport_init(rhport, &rh_init)) {
        log_error("Failed to initialize USB device");
    }
}

void usb_device_send_report(
    const uint8_t kvm_hid_idx,
    const uint8_t report_id,
    const uint8_t* report_data,
    const uint8_t report_data_len
) {
    if (!tud_hid_n_ready(kvm_hid_idx)) {
        logf_warning("tud_hid_ready(%u) == false, skip report", kvm_hid_idx);
        return;
    }
    if (!tud_hid_n_report(kvm_hid_idx, report_id, report_data, report_data_len)) {
        log_error("tud_hid_n_report failed");
    }
}

// ╔══════════════════════════════════╗
// ║         tinyusb callbacks        ║
// ╚══════════════════════════════════╝

// ReSharper disable CppParameterNamesMismatch

// ┌──────────────────────────────────┐
// │         Initialization           │
// └──────────────────────────────────┘

/*
┌──────────────────┐              ┌──────────────────┐
│      Device      │              │      String      │
│    descriptor    │              │    descriptor    │
└────────┬─────────┘              └──────────────────┘
         │
         v
┌──────────────────┐
│  Configuration   │
│    descriptor    │
└────────┬─────────┘
         │
         v
┌──────────────────┐
│    Interface     │
│    descriptor    │
└────────┬─────────┘
         │
         ├────────────────────────────┐
         │                            │
         v                            v
┌──────────────────┐        ┌──────────────────┐
│     Endpoint     │        │       HID        │
│    descriptor    │        │    descriptor    │
└──────────────────┘        └────────┬─────────┘
                                     │
                            ┌────────┴────────┐
                            │                 │
                            v                 v
                   ┌──────────────────┐  ┌──────────────────┐
                   │      Report      │  │     Physical     │
                   │    descriptor    │  │    descriptor    │
                   └──────────────────┘  └──────────────────┘
*/

// ── String descriptor ────────────────────────────────────────

// String Descriptor Index
enum {
    STRID_LANGID = 0,
    STRID_MANUFACTURER,
    STRID_PRODUCT,
    STRID_SERIAL,
};

// array of pointer to string descriptors
static char const *string_descriptor_array[] =
{
    (const char[]){0x09, 0x04}, // 0: is supported language is English (0x0409)
    "Socolin", // 1: Manufacturer
    "KVM Switch", // 2: Product
    NULL, // 3: Serials will use unique ID if possible
};

static uint16_t desc_str[32 + 1];

uint16_t const *tud_descriptor_string_cb(
    const uint8_t index,
    const uint16_t langid
) {
    (void) langid;
    size_t chr_count;

    switch (index) {
        case STRID_LANGID:
            memcpy(&desc_str[1], string_descriptor_array[0], 2);
            chr_count = 1;
            break;
        case STRID_SERIAL:
            chr_count = board_usb_get_serial(desc_str + 1, 32);
            break;
        default:
            // Note: the 0xEE index string is a Microsoft OS 1.0 Descriptors.
            // https://docs.microsoft.com/en-us/windows-hardware/drivers/usbcon/microsoft-defined-usb-descriptors

            if (!(index < sizeof(string_descriptor_array) / sizeof(string_descriptor_array[0])))
                return nullptr;

            const char *str = string_descriptor_array[index];

            // Cap at max char
            chr_count = strlen(str);
            constexpr size_t max_count = sizeof(desc_str) / sizeof(desc_str[0]) - 1; // -1 for string type
            if (chr_count > max_count) chr_count = max_count;

            // Convert ASCII string into UTF-16
            for (size_t i = 0; i < chr_count; i++) {
                desc_str[1 + i] = str[i];
            }
            break;
    }

    // first byte is length (including header), second byte is string type
    desc_str[0] = (uint16_t) ((TUSB_DESC_STRING << 8) | (2 * chr_count + 2));

    return desc_str;
}

// ── Device descriptor ────────────────────────────────────────

#define USB_BCD   0x0200

static tusb_desc_device_t device_descriptor =
{
    .bLength = sizeof(tusb_desc_device_t),
    .bDescriptorType = TUSB_DESC_DEVICE,
    .bcdUSB = USB_BCD,
    .bDeviceClass = 0x00,
    .bDeviceSubClass = 0x00,
    .bDeviceProtocol = 0x00,
    .bMaxPacketSize0 = CFG_TUD_ENDPOINT0_SIZE,

    .idVendor = 0,
    .idProduct = 0,
    .bcdDevice = 0x0100,

    .iManufacturer = STRID_MANUFACTURER,
    .iProduct = STRID_PRODUCT,
    .iSerialNumber = STRID_SERIAL,

    .bNumConfigurations = 0x01
};

uint8_t const *tud_descriptor_device_cb() {
    device_descriptor.idVendor = usb_device.vid;
    device_descriptor.idProduct = usb_device.pid;
    return (uint8_t const *) &device_descriptor;
}

// ── Device Qualifier descriptor ──────────────────────────────

#if TUD_OPT_HIGH_SPEED
// Per USB specs: high speed capable device must report device_qualifier and other_speed_configuration

// other speed configuration
uint8_t desc_other_speed_config[CONFIG_TOTAL_LEN];

// device qualifier is mostly similar to device descriptor since we don't change configuration based on speed
tusb_desc_device_qualifier_t const desc_device_qualifier =
{
    .bLength = sizeof(tusb_desc_device_qualifier_t),
    .bDescriptorType = TUSB_DESC_DEVICE_QUALIFIER,
    .bcdUSB = USB_BCD,

    .bDeviceClass = 0x00,
    .bDeviceSubClass = 0x00,
    .bDeviceProtocol = 0x00,

    .bMaxPacketSize0 = CFG_TUD_ENDPOINT0_SIZE,
    .bNumConfigurations = 0x01,
    .bReserved = 0x00
};

// Invoked when received GET DEVICE QUALIFIER DESCRIPTOR request
// Application return pointer to descriptor, whose contents must exist long enough for transfer to complete.
// device_qualifier descriptor describes information about a high-speed capable device that would
// change if the device were operating at the other speed. If not highspeed capable stall this request.
uint8_t const *tud_descriptor_device_qualifier_cb(void) {
    return (uint8_t const *) &desc_device_qualifier;
}

// Invoked when received GET OTHER SEED CONFIGURATION DESCRIPTOR request
// Application return pointer to descriptor, whose contents must exist long enough for transfer to complete
// Configuration descriptor in the other speed e.g if high speed then this is for full speed and vice versa
uint8_t const *tud_descriptor_other_speed_configuration_cb(uint8_t index) {
    (void) index; // for multiple configurations

    // other speed config is basically configuration with type = OHER_SPEED_CONFIG
    memcpy(desc_other_speed_config, desc_configuration, CONFIG_TOTAL_LEN);
    desc_other_speed_config[1] = TUSB_DESC_OTHER_SPEED_CONFIG;

    // this example use the same configuration for both high and full speed mode
    return desc_other_speed_config;
}

#endif // highspeed

// ── Configuration and Interface descriptor ───────────────────

#define CONFIG_TOTAL_LEN  (TUD_CONFIG_DESC_LEN + TUD_HID_DESC_LEN * CFG_TUD_HID)
// Use endpoint 1 as output (& 0x80). Endpoint 0 is reserved for control.
#define FIST_ENDPOINT_NUM_HID (0x80 | 0x01)
static uint8_t configuration_descriptor[CONFIG_TOTAL_LEN];

uint8_t const *tud_descriptor_configuration_cb(uint8_t index) {
    (void) index; // for multiple configurations

    constexpr uint8_t base_config_descriptor[] =
    {
        // Config number, interface count, string index, total length, attribute, power in mA
        // FIXME: Should the 100 mA be configurable based on the HID connected ?
        TUD_CONFIG_DESCRIPTOR(1, CFG_TUD_HID, 0, CONFIG_TOTAL_LEN, TUSB_DESC_CONFIG_ATT_REMOTE_WAKEUP, 100),
    };
    memcpy(configuration_descriptor, base_config_descriptor, sizeof(base_config_descriptor));

    size_t offset = sizeof(base_config_descriptor);
    for (uint8_t kvm_hid_idx = 0; kvm_hid_idx < (uint8_t) CFG_TUD_HID; kvm_hid_idx++) {
        // If no hid is connected to this interface, we send an empty descriptor
        const hid_t *hid = hid_mgr_get_by_kvm_idx(kvm_hid_idx);
        uint8_t itf_protocol = HID_ITF_PROTOCOL_NONE;
        uint16_t report_desc_len = 0;
        if (hid) {
            itf_protocol = hid->itf_protocol;
            report_desc_len = hid->report_desc_len;
        }

        const uint8_t itf_config_descriptor[] =
        {
            // Interface number, string index, protocol, report descriptor len, EP In address, size & polling interval
            TUD_HID_DESCRIPTOR(kvm_hid_idx, 0, itf_protocol, report_desc_len, FIST_ENDPOINT_NUM_HID + kvm_hid_idx,
                               CFG_TUD_HID_EP_BUFSIZE, 5),
        };
        memcpy(configuration_descriptor + offset, itf_config_descriptor, sizeof(itf_config_descriptor));
        offset += sizeof(itf_config_descriptor);
    }

    assert(offset == sizeof(configuration_descriptor));

    return configuration_descriptor;
}

// ── HID descriptor (Include Report descriptor) ───────────────

static constexpr uint8_t empty_hid_descriptor[] = {};

uint8_t const *tud_hid_descriptor_report_cb(
    const uint8_t kvm_hid_idx
) {
    const hid_t *hid = hid_mgr_get_by_kvm_idx(kvm_hid_idx);
    if (hid == nullptr)
        return empty_hid_descriptor;
    return hid->report_desc;
}

// ┌──────────────────────────────────┐
// │          Working state           │
// └──────────────────────────────────┘

void tud_mount_cb() {
    logf_debug("device mounted");

    for (uint8_t kvm_hid_idx = 0; kvm_hid_idx < hid_mgr_get_max_hid_count(); kvm_hid_idx++) {
        const hid_t *hid = hid_mgr_get_by_kvm_idx(kvm_hid_idx);
        if (hid == nullptr) {
            continue;
        }
        // Only keyboard and mouse support boot protocol
        if (hid->itf_protocol == HID_ITF_PROTOCOL_NONE)
            continue;

        const uint8_t hid_protocol = tud_hid_n_get_protocol(kvm_hid_idx);
        logf_debug("HID protocol for device: %u interface: %u is: %u", hid->dev_addr, hid->host_hid_idx, hid_protocol);

        usb_device.set_computer_hid_protocol_cb(usb_device.computer_id, kvm_hid_idx, hid_protocol);
    }
}

void tud_umount_cb() {
    logf_debug("device unmounted");

    // FIXME: Reset computer state
}

uint16_t tud_hid_get_report_cb(
    const uint8_t kvm_hid_idx,
    const uint8_t report_id,
    const hid_report_type_t report_type,
    uint8_t *buffer,
    const uint16_t reqlen
) {
    logf_debug("kvm_hid_idx: %u, report_id: %u, report_type: %u", kvm_hid_idx, report_id, report_type);

    // FIXME: Is this needed ?
    memset(buffer, 0, reqlen);

    return 0;
}

/**
 * This is called by the computer when it sends a HID report, e.g. update keyboard LEDs
 * @param kvm_hid_idx The index of the HID interface in the KVM (Exposed to the computers)
 * @param report_id The HID report ID
 * @param report_type The HID report type
 * @param report_data The HID report
 * @param report_data_len The HID report size
 */
void tud_hid_set_report_cb(
    const uint8_t kvm_hid_idx,
    const uint8_t report_id,
    const hid_report_type_t report_type,
    uint8_t const *report_data,
    const uint16_t report_data_len
) {
    logf_debug("kvm_hid_idx: %u, report_id: %u, report_type: %u", kvm_hid_idx, report_id, report_type);
    log_debug_hex_buffer(report_data, report_data_len);

    usb_device.set_computer_report_cb(usb_device.computer_id, kvm_hid_idx, report_id, report_type, report_data, report_data_len);
}

/**
 * This is called by the computer when it changes the HID protocol (Boot or Report)
 * @param kvm_hid_idx The index of the HID interface in the KVM (Exposed to the computers)
 * @param hid_protocol The HID protocol (boot / report) \see hid_protocol_mode_enum_t
 */
void tud_hid_set_protocol_cb(
    const uint8_t kvm_hid_idx,
    const uint8_t hid_protocol
) {
    logf_debug("kvm_hid_idx:%u hid_protocol:%u", kvm_hid_idx, hid_protocol);

    usb_device.set_computer_hid_protocol_cb(usb_device.computer_id, kvm_hid_idx, hid_protocol);
}
