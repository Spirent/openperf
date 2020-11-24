
#ifndef _OP_LOG_H_
#define _OP_LOG_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdarg.h>
#include <stdint.h>
#include <string.h>

enum op_log_level {
    OP_LOG_NONE = 0,
    OP_LOG_CRITICAL, /**< Fatal error condition */
    OP_LOG_ERROR,    /**< Non-fatal error condition */
    OP_LOG_WARNING,  /**< Unexpected event or condition */
    OP_LOG_INFO,     /**< Informational messages */
    OP_LOG_DEBUG,    /**< Debugging messages */
    OP_LOG_TRACE,    /**< Trace level messages */
    OP_LOG_MAX,
};

/**
 * Get the application log level
 *
 * @return
 *   The current system log level
 */
enum op_log_level op_log_level_get(void) __attribute__((pure));

/**
 * Set the application log level
 *
 * @param level
 *   A value between OP_LOG_ERROR (1) and OP_LOG_TRACE (6)
 */
void op_log_level_set(enum op_log_level level);

/**
 * Retrieve the log level from the command line
 *
 * @param[in] argc
 *   The number of cli arguments
 * @param[in] argv
 *   Array of cli strings
 *
 * @return
 *   log level found in cli arguments (may be OP_LOG_NONE)
 */
enum op_log_level op_log_level_find(int argc, char* const argv[]);

/**
 * Maximum length (in chars) of an OP log level value.
 */
static const size_t OP_LOG_MAX_LEVEL_LENGTH = 8;

/**
 * Parse a log level argument to the associated enum value.
 *
 * @param[in] arg
 *   Log level argument to parse
 *
 * @return
 *   log level found in arg, OP_LOG_NONE otherwise
 */
enum op_log_level parse_log_optarg(const char* arg);

/**
 * Get the full function name from the full function signature string
 *
 * @param[in] signature
 *   The full function signature
 * @parma[out] function
 *   Buffer for function name; should be at least as long as signature
 */
void op_log_function_name(const char* signature, char* function);

/**
 * Macro to possibly write a message to the log
 * Note: this is the preferred way to do logging, since logging arguments will
 * not be evaluated unless they will actually get logged.
 *
 * @param level
 *   The level of the message
 * @param format
 *   The printf format string, followed by variable arguments
 * @return
 *   -  0: Success
 *   - !0: Error
 */
#define OP_LOG(level, format, ...)                                             \
    do {                                                                       \
        if (level <= op_log_level_get()) {                                     \
            char function_[strlen(__PRETTY_FUNCTION__)];                       \
            op_log_function_name(__PRETTY_FUNCTION__, function_);              \
            op_log(level, function_, format, ##__VA_ARGS__);                   \
        }                                                                      \
    } while (0)

/**
 * Write a message to the log
 *
 * @param level
 *   The level of the message
 * @param tag
 *   Additional information to add to message
 * @param format
 *   The printf format string, followed by variable arguments
 * @return
 *   -  0: Success
 *   - !0: Error
 */
int op_log(enum op_log_level level, const char* tag, const char* format, ...)
    __attribute__((format(printf, 3, 4)));

int op_vlog(enum op_log_level level,
            const char* tag,
            const char* format,
            va_list argp);

/**
 * Explicit close the logging socket of the calling thread.
 */
void op_log_close(void);

/**
 * Intialize the logging subsystem
 *
 * @param context
 *   The 0mq context to use for creating the logging socket
 * @return
 *   - 0: Success
 *   -!0: Error
 */
int op_log_init(void* context, const char* logging_endpoint);

/**
 * Cleanly release all remaining logging resources
 */
void op_log_finish();

#ifdef __cplusplus
}
#endif

#endif
