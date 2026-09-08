#include "logger.h"

#include <ctype.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/unistd.h>

#include "ring_buffer.h"
#include "hardware/timer.h"

typedef struct __attribute__((packed)) {
    uint64_t timestamp;
    uint8_t log_level; // log_level_t
    uint16_t line;
    uint8_t func_len;
    uint8_t message_len;
} internal_log_t;

typedef struct {
    ring_buffer_t ring_buffer;
    uint8_t log_buffer[8192];
    uint8_t min_log_level;
    uint8_t immediate_log_min_log_level;
    size_t dropped_logs;
} logger_t;

static logger_t default_logger[NUM_CORES];

void logger_init(
    const log_level_t min_log_level,
    const log_level_t immediate_log_min_log_level
) {
    for (size_t c = 0; c < NUM_CORES; c++) {
        memset(&default_logger[c], 0, sizeof(logger_t));
        logger_t *logger = &default_logger[c];
        logger->min_log_level = min_log_level;
        logger->immediate_log_min_log_level = immediate_log_min_log_level;
        ring_buffer_init(&logger->ring_buffer, logger->log_buffer, sizeof(logger->log_buffer));
    }
}


static bool ensure_enough_room_available_in_log_buffer(
    logger_t *logger,
    const size_t len
) {
    if (len >= sizeof(logger->log_buffer))
        return false;

    // If the buffer is full, drop the oldest log from the buffer
    while (ring_buffer_get_available_space(&logger->ring_buffer) < len) {
        logger->dropped_logs++;

        internal_log_t log = {0};
        ring_buffer_read(&logger->ring_buffer, (uint8_t *) &log, sizeof(log));
        ring_buffer_read(&logger->ring_buffer, nullptr, log.func_len);
        ring_buffer_read(&logger->ring_buffer, nullptr, log.message_len);
    }

    return true;
}

#if LOG_COLOR
static const char *log_level_with_color_to_string(uint8_t log_level) {
    switch (log_level) {
        case LOG_LEVEL_CRITICAL: return "\033[1;35mCRI\033[0m";
        case LOG_LEVEL_ERROR: return "\033[1;31mERR\033[0m";
        case LOG_LEVEL_WARNING: return "\033[1;33mWRN\033[0m";
        case LOG_LEVEL_INFO: return "\033[1;37mINF\033[0m";
        case LOG_LEVEL_DEBUG: return "\033[1;36mDBG\033[0m";
        default: return "\033[1;90mUNK\033[0m";
    }
}
#else
static const char *log_level_to_string(uint8_t log_level) {
    switch (log_level) {
        case LOG_LEVEL_CRITICAL: return "CRI";
        case LOG_LEVEL_ERROR: return "ERR";
        case LOG_LEVEL_WARNING: return "WRN";
        case LOG_LEVEL_INFO: return "INF";
        case LOG_LEVEL_DEBUG: return "DBG";
        default: return "UNK";
    }
}
#endif

// ┌──────────────────────────────────┐
// │            Simple log            │
// └──────────────────────────────────┘

void log_write(
    const uint8_t log_level,
    const char *func,
    const uint16_t line,
    const char *message,
    size_t message_len
) {
    if (message_len > 255)
        message_len = 255;

    const internal_log_t log = {
        .timestamp = time_us_64(),
        .log_level = log_level,
        .line = line,
        .func_len = strlen(func),
        .message_len = message_len,
    };
    const uint core_id = get_core_num();
    logger_t *logger = &default_logger[core_id];
    if (logger->min_log_level <= log_level) {
        if (ensure_enough_room_available_in_log_buffer(logger, sizeof(log) + log.func_len + message_len)) {
            if (ring_buffer_write(&logger->ring_buffer, (const uint8_t *) &log, sizeof(log)) != sizeof(log)) {
                panic("Failed to write log header to ring buffer");
            }
            if (ring_buffer_write(&logger->ring_buffer, (const uint8_t *) func, log.func_len) != log.func_len) {
                panic("Failed to write log function name to ring buffer");
            }
            if (ring_buffer_write(&logger->ring_buffer, (const uint8_t *) message, message_len) != message_len) {
                panic("Failed to write log message to ring buffer");
            }
        } else {
            logger->dropped_logs++;
        }
    }
    if (logger->immediate_log_min_log_level <= log_level) {
#if LOG_COLOR
        printf("[%u][%llu][%s][%s:%u] %.*s\n", core_id, log.timestamp, log_level_with_color_to_string(log_level), func,
               log.line, message_len,
               message);
#else
        printf("[%u][%llu][%s][%s:%u] %.*s\n", core_id, log.timestamp, log_level_to_string(log_level), func, log.line,
               message_len, message);
#endif
    }
}

// ┌──────────────────────────────────┐
// │            Log buffer            │
// └──────────────────────────────────┘

void log_hex_buffer(
    const uint8_t log_level,
    const char *func,
    const uint16_t line,
    const void *data,
    const size_t data_len
) {
    char message_buffer[256];
    for (size_t data_offset = 0; data_offset < data_len; data_offset += 16) {
        snprintf(message_buffer, sizeof(message_buffer), "%04x  ", data_offset);
        size_t str_size = 6;
        for (size_t i = 0; i < 16; i++) {
            if (data_offset + i < data_len)
                snprintf(message_buffer + str_size, 4, "%02x ", ((const uint8_t *) data)[data_offset + i]);
            else
                snprintf(message_buffer + str_size, 4, "   ");
            str_size += 3;
            if (i == 7) {
                snprintf(message_buffer + str_size, 2, " ");
                str_size += 1;
            }
        }
        snprintf(message_buffer + str_size, 4, "  |");
        str_size += 3;
        for (size_t i = 0; i < 16; i++) {
            if (data_offset + i < data_len) {
                const uint8_t c = ((const uint8_t *) data)[data_offset + i];
                message_buffer[str_size] = isprint(c) ? c : '.';
                str_size += 1;
            }
        }
        snprintf(message_buffer + str_size, 4, " |");
        str_size += 2;
        message_buffer[str_size] = '\0';
        log_write(log_level, func, line, message_buffer, str_size);
    }
}

// ┌──────────────────────────────────┐
// │         Log with format          │
// └──────────────────────────────────┘

void log_write_format(
    const uint8_t log_level,
    const char *func,
    const uint16_t line,
    const char *message,
    ...
) {
    char message_buffer[256];
    va_list args;
    va_start(args, message);
    int message_len = vsnprintf(message_buffer, sizeof(message_buffer), message, args);
    va_end(args);

    if (message_len < 0)
        return;
    if (message_len >= (int) sizeof(message_buffer))
        message_len = sizeof(message_buffer) - 1;

    const internal_log_t log = {
        .timestamp = time_us_64(),
        .log_level = log_level,
        .func_len = strlen(func),
        .line = line,
        .message_len = message_len,
    };
    const uint core_id = get_core_num();
    logger_t *logger = &default_logger[core_id];
    if (logger->min_log_level <= log_level) {
        if (ensure_enough_room_available_in_log_buffer(logger, sizeof(log) + log.func_len + message_len)) {
            if (ring_buffer_write(&logger->ring_buffer, (const uint8_t *) &log, sizeof(log)) != sizeof(log))
                panic("Failed to write log header to ring buffer");
            if (ring_buffer_write(&logger->ring_buffer, (const uint8_t *) func, log.func_len) != log.func_len)
                panic("Failed to write log function name to ring buffer");
            if (ring_buffer_write(&logger->ring_buffer, (const uint8_t *) message_buffer, message_len) != (size_t)message_len)
                panic("Failed to write log message to ring buffer");
        } else {
            logger->dropped_logs++;
        }
    }
    if (logger->immediate_log_min_log_level <= log_level) {
#if LOG_COLOR
        printf("[%u][%llu][%s][%s:%u] %.*s\n", core_id, log.timestamp, log_level_with_color_to_string(log_level), func,
               line, message_len,
               message_buffer);
#else
        printf("[%u][%llu][%s][%s:%u] %.*s\n", core_id, log.timestamp, log_level_to_string(log_level), func, log.line,
               message_len,
               message_buffer);
#endif
    }
}

// ┌──────────────────────────────────┐
// │             Read logs            │
// └──────────────────────────────────┘

bool try_dequeue_log(
    log_t *out_log
) {
    logger_t *logger = nullptr;
    uint64_t min_timestamp = (uint64_t) -1;
    for (size_t c = 0; c < NUM_CORES; c++) {
        internal_log_t log = {0};
        logger_t *core_logger = &default_logger[c];
        if (ring_buffer_get_length(&core_logger->ring_buffer) == 0)
            continue;
        const size_t read_count = ring_buffer_peek(&core_logger->ring_buffer, (uint8_t *) &log, sizeof(log));
        if (read_count == 0)
            continue;
        if (log.timestamp < min_timestamp) {
            min_timestamp = log.timestamp;
            logger = core_logger;
        }
    }

    if (logger == nullptr)
        return false;

    // Read metadata
    internal_log_t log = {0};
    const size_t read_count = ring_buffer_read(
        &logger->ring_buffer,
        (uint8_t *) &log,
        sizeof(log)
    );
    if (read_count != sizeof(log))
        return false;

    out_log->timestamp = log.timestamp;
    out_log->log_level = log.log_level;
    out_log->line = log.line;

    // Read func
    const size_t func_read_count = ring_buffer_read(
        &logger->ring_buffer,
        out_log->func,
        log.func_len
    );
    out_log->func_len = func_read_count;
    out_log->func[func_read_count] = '\0';

    // Read message
    const size_t message_read_count = ring_buffer_read(
        &logger->ring_buffer,
        out_log->msg,
        log.message_len
    );
    out_log->msg_len = message_read_count;
    out_log->msg[message_read_count] = '\0';

    return true;
}
