
#ifndef _ICP_LOG_H_
#define _ICP_LOG_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdarg.h>
#include <stdint.h>
#include <string.h>

enum icp_log_level {
    ICP_LOG_NONE = 0,
    ICP_LOG_CRITICAL,     /**< Fatal error condition */
    ICP_LOG_ERROR,        /**< Non-fatal error condition */
    ICP_LOG_WARNING,      /**< Unexpected event or condition */
    ICP_LOG_INFO,         /**< Informational messages */
    ICP_LOG_DEBUG,        /**< Debugging messages */
    ICP_LOG_TRACE,        /**< Trace level messages */
    ICP_LOG_MAX,
};

/**
 * Get the application log level
 *
 * @return
 *   The current system log level
 */
enum icp_log_level icp_log_level_get(void) __attribute__((pure));

/**
 * Set the application log level
 *
 * @param level
 *   A value between ICP_LOG_ERROR (1) and ICP_LOG_TRACE (6)
 */
void icp_log_level_set(enum icp_log_level level);

/**
 * Retrieve the log level from the command line
 *
 * @param[in] argc
 *   The number of cli arguments
 * @param[in] argv
 *   Array of cli strings
 *
 * @return
 *   log level found in cli arguments (may be ICP_LOG_NONE)
 */
enum icp_log_level icp_log_level_find(int argc, char * const argv[]);

/**
 * Get the full function name from the full function signature string
 *
 * @param[in] signature
 *   The full function signature
 * @parma[out] function
 *   Buffer for function name; should be at least as long as signature
 */
void icp_log_function_name(const char *signature, char *function);

/**
 * Possibly write a message to the log
 *
 * @param level
 *   The level of the message
 * @param format
 *   The printf format string, followed by variable arguments
 * @return
 *   -  0: Success
 *   - !0: Error
 */
#define icp_log(level, format, ...)                                     \
    do {                                                                \
        if (level <= icp_log_level_get()) {                             \
            char function_[strlen(__PRETTY_FUNCTION__)];                \
            icp_log_function_name(__PRETTY_FUNCTION__, function_);      \
            _icp_log(level, function_, format, ##__VA_ARGS__);          \
        }                                                               \
    } while (0)

int _icp_log(enum icp_log_level level, const char *tag,
             const char *format, ...)
    __attribute__((format(printf, 3, 4)));

int icp_vlog(enum icp_log_level level, const char *tag,
             const char *format, va_list argp);

/**
 * Intialize the logging subsystem
 *
 * @param context
 *   The 0mq context to use for creating the logging socket
 * @return
 *   - 0: Success
 *   -!0: Error
 */
int icp_log_init(void *context, const char *logging_endpoint);

#ifdef __cplusplus
}
#endif

#endif
