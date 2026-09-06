#pragma once

#include <stddef.h>
#include <stdint.h>
// ReSharper disable once CppUnusedIncludeDirective used by strlen() during macro expansion
#include <string.h>

#define LOG_COLOR 1

typedef enum {
    LOG_LEVEL_DEBUG,
    LOG_LEVEL_INFO,
    LOG_LEVEL_WARNING,
    LOG_LEVEL_ERROR,
    LOG_LEVEL_CRITICAL,
} log_level_t;

void logger_init(
    log_level_t min_log_level,
    log_level_t immediate_log_min_log_level
);

// ┌──────────────────────────────────┐
// │            Simple log            │
// └──────────────────────────────────┘

void log_write(
    uint8_t log_level,
    const char *func,
    uint16_t line,
    const char *message,
    size_t message_len
);

#define log_critical(message) \
    log_write(LOG_LEVEL_CRITICAL, __func__, __LINE__, message, strlen(message))

#define log_error(message) \
    log_write(LOG_LEVEL_ERROR, __func__, __LINE__, message, strlen(message))

#define log_warning(message) \
    log_write(LOG_LEVEL_WARNING, __func__, __LINE__, message, strlen(message))

#define log_info(message) \
    log_write(LOG_LEVEL_INFO, __func__, __LINE__, message, strlen(message))

#define log_debug(message) \
    log_write(LOG_LEVEL_DEBUG, __func__, __LINE__, message, strlen(message))

// ┌──────────────────────────────────┐
// │            Log buffer            │
// └──────────────────────────────────┘

void log_hex_buffer(
    uint8_t log_level,
    const char *func,
    uint16_t line,
    const void *data,
    size_t data_len
);

#define log_critical_hex_buffer(data, data_len) \
    log_hex_buffer(LOG_LEVEL_CRITICAL, __func__, __LINE__, data, data_len)

#define log_error_hex_buffer(data, data_len) \
    log_hex_buffer(LOG_LEVEL_ERROR, __func__, __LINE__, data, data_len)

#define log_warning_hex_buffer(data, data_len) \
    log_hex_buffer(LOG_LEVEL_WARNING, __func__, __LINE__, data, data_len)

#define log_info_hex_buffer(data, data_len) \
    log_hex_buffer(LOG_LEVEL_INFO, __func__, __LINE__, data, data_len)

#define log_debug_hex_buffer(data, data_len) \
    log_hex_buffer(LOG_LEVEL_DEBUG, __func__, __LINE__, data, data_len)

// ┌──────────────────────────────────┐
// │         Log with format          │
// └──────────────────────────────────┘

void log_write_format(
    uint8_t log_level,
    const char *func,
    uint16_t line,
    const char *message,
    ...
) __attribute__((format(printf, 4, 5)));

#define logf_critical(message, ...) \
    log_write_format(LOG_LEVEL_CRITICAL, __func__, __LINE__, message, ##__VA_ARGS__)

#define logf_error(message, ...) \
    log_write_format(LOG_LEVEL_ERROR, __func__, __LINE__, message, ##__VA_ARGS__)

#define logf_warning(message, ...) \
    log_write_format(LOG_LEVEL_WARNING, __func__, __LINE__, message, ##__VA_ARGS__)

#define logf_info(message, ...) \
    log_write_format(LOG_LEVEL_INFO, __func__, __LINE__, message, ##__VA_ARGS__)

#define logf_debug(message, ...) \
    log_write_format(LOG_LEVEL_DEBUG, __func__, __LINE__, message, ##__VA_ARGS__)

// ┌──────────────────────────────────┐
// │             Read logs            │
// └──────────────────────────────────┘

typedef struct __attribute__((packed)) {
    uint64_t timestamp;
    uint8_t log_level; // log_level_t
    uint16_t line;
    uint8_t func_len; // 0 - 255 (\0 not included)
    uint8_t func[256];
    uint8_t msg_len; // 0 - 255 (\0 not included)
    uint8_t msg[256];
} log_t;

bool try_dequeue_log(log_t *out_log);
