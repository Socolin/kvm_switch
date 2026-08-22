#include <stdint.h>

static inline uint16_t max16(const uint16_t x, const uint16_t y) { return (x > y) ? x : y; }
static inline uint32_t max32(const uint32_t x, const uint32_t y) { return (x > y) ? x : y; }
