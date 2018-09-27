#ifndef _ICP_CONFIG_SERVER_H_
#define _ICP_CONFIG_SERVER_H_

/**
 * @file
 */

#include <errno.h>
#include "core/icp_config.h"

/**
 * Enumeration for specyfing service requests
 */
enum icp_config_server_request_type {
    REQUEST_PING = 1,
    REQUEST_REG,
    REQUEST_ADD,
    REQUEST_DEL,
    REQUEST_GET,
    REQUEST_SET,
};

/**
 * Enumeration for response code included in all service replies
 */
enum icp_config_server_response_code {
    RESPONSE_OK = 0,
    RESPONSE_ERROR = -1,
    RESPONSE_TRY_AGAIN = -EAGAIN,
    RESPONSE_UNKNOWN_KEY = -EINVAL,
};

enum icp_config_server_flags {
    CONFIG_FLAG_NONE = 0x00,
    CONFIG_FLAG_ADD  = 0x01,
    CONFIG_FLAG_DEL  = 0x02,
    CONFIG_FLAG_GET  = 0x04,
    CONFIG_FLAG_SET  = 0x08,
};

/**
 * Structure describing a configuration service request
 */
struct icp_config_server_request {
    enum icp_config_server_request_type type;  /**< Type of request */
    const char *key;                           /**< Node to perform request on */
    union {
        struct {
            const char *endpoint;              /**< ZeroMQ endpoint that owns this key (register) */
            int flags;                         /**< valid operations for this key */
        };
        struct icp_config_data data;           /**< Data for operation (adds/sets only) */
    };
};

/**
 * Structure describing a configuration service reply
 */
struct icp_config_server_reply {
    enum icp_config_server_response_code code;  /**< Response indicating success/failure */
    const char *key;                            /**< Configuration node of operation */
    struct icp_config_data data;                /**< Data of request (gets) */
};

#endif /* _ICP_CONFIG_SERVER_H_ */
