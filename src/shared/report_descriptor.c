#include "report_descriptor.h"

// See hid1_11.pdf
bool is_report_id_present_in_descriptor(
    const uint8_t *report_desc,
    const uint16_t desc_len
) {
    uint16_t i = 0;

    while (i < desc_len) {
        const uint8_t prefix = report_desc[i++];

        // Skip unused "Long items"
        if (prefix == 0xFE) {
            if (i + 2 > desc_len) {
                return false;
            }

            const uint8_t data_size = report_desc[i++];
            [[maybe_unused]] const uint8_t long_tag = report_desc[i++];

            if (i + data_size > desc_len) {
                return false;
            }

            i += data_size;
            continue;
        }

        const uint8_t size_code = prefix & 0x3;
        const uint8_t type = (prefix >> 2) & 0x3;
        const uint8_t tag = (prefix >> 4) & 0xf;

        uint8_t data_size = 0;
        switch (size_code) {
            case 0: data_size = 0;
                break;
            case 1: data_size = 1;
                break;
            case 2: data_size = 2;
                break;
            case 3: data_size = 4;
                break;
            default:
                return false;
        }

        if (i + data_size > desc_len)
            return false;

        if (type == 1 /*RI_TYPE_GLOBAL*/ && tag == 8 /*RI_GLOBAL_REPORT_ID*/) {
            return true;
        }

        i += data_size;
    }

    return false;
}
