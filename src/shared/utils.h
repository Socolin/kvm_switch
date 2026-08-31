#pragma once

#include <stdint.h>

static inline uint16_t max16(const uint16_t x, const uint16_t y) { return (x > y) ? x : y; }
static inline uint32_t max32(const uint32_t x, const uint32_t y) { return (x > y) ? x : y; }

#ifndef htole32
#define htole32(u32) (u32)
#endif
#ifndef le32toh
#define le32toh(u32) (u32)
#endif

#define min(x, y) (x < y ? x : y)
