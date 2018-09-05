#ifndef _ICP_LOG_H_
#define _ICP_LOG_H_

#include <stdarg.h>
#include <stdint.h>

enum icp_log_level {
    ICP_LOG_NONE = 0,
    ICP_LOG_CRITICAL,     /**< Fatal error condition */
    ICP_LOG_ERROR,        /**< Non-fatal error condition */
    ICP_LOG_WARNING,      /**< Unexpected event or condition */
    ICP_LOG_INFO,         /**< Informational messages */
    ICP_LOG_DEBUG,        /**< Debugging messages */
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
 *   A value between ICP_LOG_ERROR (1) and ICP_LOG_DEBUG (5)
 */
void icp_log_level_set(enum icp_log_level level);

/**
 * Possibly write a message to the log
 *
 * @param level
 *   The level of the message
 * @param format
 *   The printf format string, followed by  variable arguments
 * @return
 *   - 0: Success
 *   - < 0: Error
 */
#define icp_log(level, format, ...)                             \
    do {                                                        \
        if (level <= icp_log_level_get()) {                     \
            _icp_log(level, __func__, format, ##__VA_ARGS__);   \
        }                                                       \
    } while (0)

int _icp_log(enum icp_log_level level, const char *function,
             const char *format, ...)
    __attribute__((format(printf, 3, 4)));

int icp_vlog(enum icp_log_level level, const char *function,
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

#endif
