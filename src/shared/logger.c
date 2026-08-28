#include "logger.h"

#include <ctype.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/unistd.h>

#include "hardware/timer.h"

typedef struct __attribute__((packed)) {
    const uint64_t timestamp;
    const uint8_t log_level; // log_level_t
    const uint16_t line;
    const uint8_t func_len;
    const uint8_t message_len;
} internal_log_t;

typedef struct {
    char log_buffer[4096];
    __uint16_t start_position;
    __uint16_t end_position;
    uint8_t min_log_level;
    uint8_t immediate_log_min_log_level;
} logger_t;

static logger_t default_logger[NUM_CORES];

void logger_init(
    const log_level_t min_log_level,
    const log_level_t immediate_log_min_log_level
) {
    for (size_t c = 0; c < NUM_CORES; c++) {
        memset(&default_logger[c], 0, sizeof(logger_t));
        default_logger[c].min_log_level = min_log_level;
        default_logger[c].immediate_log_min_log_level = immediate_log_min_log_level;
    }
}

static size_t read_bytes_from_log_buffer(
    logger_t *logger,
    const size_t len,
    uint8_t *bytes,
    const size_t bytes_len
) {
    if (logger->start_position == logger->end_position)
        return 0;
    size_t read_count = 0;
    while (read_count < len) {
        if (bytes && read_count < bytes_len) {
            bytes[read_count] = logger->log_buffer[logger->start_position];
            read_count++;
        }
        logger->start_position = (logger->start_position + 1) % sizeof(logger->log_buffer);
    }
    return read_count;
}

static size_t peek_bytes_from_log_buffer(
    const logger_t *logger,
    uint8_t *bytes,
    const size_t len
) {
    if (logger->start_position == logger->end_position)
        return 0;
    size_t read_count = 0;
    while (read_count < len) {
        bytes[read_count] = logger->log_buffer[(logger->start_position + read_count) % sizeof(logger->log_buffer)];
        read_count++;
    }
    return read_count;
}

static void write_bytes_to_log_buffer(
    logger_t *logger,
    const uint8_t *bytes,
    const size_t len
) {
    size_t written = 0;
    while (written < len) {
        logger->log_buffer[logger->end_position] = bytes[written];
        written++;
        logger->end_position = (logger->end_position + 1) % sizeof(logger->log_buffer);

        // Buffer is full, discard oldest log
        if (logger->end_position == logger->start_position) {
            internal_log_t log = {};
            read_bytes_from_log_buffer(logger, sizeof(log), (uint8_t *) &log, sizeof(log));
            read_bytes_from_log_buffer(logger, log.func_len, nullptr, 0);
            read_bytes_from_log_buffer(logger, log.message_len, nullptr, 0);
        }
    }
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
    const uint8_t message_len
) {
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
        write_bytes_to_log_buffer(logger, (const uint8_t *) &log, sizeof(log));
        write_bytes_to_log_buffer(logger, (const uint8_t *) func, log.func_len);
        write_bytes_to_log_buffer(logger, (const uint8_t *) message, message_len);
    }
    if (logger->immediate_log_min_log_level <= log_level) {
#if LOG_COLOR
        printf("[%d][%llu][%s][%s:%u] %.*s\n", core_id, log.timestamp, log_level_with_color_to_string(log_level), func, log.line, message_len,
               message);
#else
        printf("[%d][%llu][%s][%s:%u] %.*s\n", core_id, log.timestamp, log_level_to_string(log_level), func, log.line, message_len, message);
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
                snprintf(message_buffer + str_size, 4, "%02x ", ((const uint8_t*)data)[data_offset + i]);
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
                const uint8_t c = ((const uint8_t*)data)[data_offset + i];
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
        write_bytes_to_log_buffer(logger, (const uint8_t *) &log, sizeof(log));
        write_bytes_to_log_buffer(logger, (const uint8_t *) func, log.func_len);
        write_bytes_to_log_buffer(logger, (const uint8_t *) message_buffer, message_len);
    }
    if (logger->immediate_log_min_log_level <= log_level) {
#if LOG_COLOR
        printf("[%d][%llu][%s][%s:%u] %.*s\n", core_id, log.timestamp, log_level_with_color_to_string(log_level), func, line, message_len,
               message_buffer);
#else
        printf("[%d][%llu][%s][%s:%u] %.*s\n", core_id, log.timestamp, log_level_to_string(log_level), func, log.line, message_len,
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
        logger_t *core_logger = &default_logger[c];
        const size_t read_count = peek_bytes_from_log_buffer(core_logger, (uint8_t *) out_log, sizeof(*out_log));
        if (read_count == 0)
            continue;
        if (out_log->timestamp < min_timestamp) {
            min_timestamp = out_log->timestamp;
            logger = core_logger;
        }
    }

    if (logger == nullptr)
        return false;

    // Read metadata
    const size_t read_count = read_bytes_from_log_buffer(
        logger,
        sizeof(*out_log),
        (uint8_t *) &out_log,
        sizeof(*out_log)
    );
    if (read_count != sizeof(log_t))
        return false;

    // Read func
    const size_t func_read_count = read_bytes_from_log_buffer(
        logger,
        out_log->func_len,
        out_log->func,
        sizeof(out_log->func) - 1
    );
    out_log->func_len = func_read_count;
    out_log->func[func_read_count] = '\0';

    // Read message
    const size_t message_read_count = read_bytes_from_log_buffer(
        logger,
        out_log->message_len,
        out_log->message,
        sizeof(out_log->message) - 1
    );
    out_log->message_len = message_read_count;
    out_log->message[message_read_count] = '\0';

    return true;
}
