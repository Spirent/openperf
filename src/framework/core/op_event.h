#ifndef _OP_EVENT_H_
#define _OP_EVENT_H_

#include <stdint.h>

struct op_event_loop;

/**
 * Structure to manage generic event data
 */
struct op_event_data {
    struct op_event_loop *loop;  /**< back pointer to event loop */
    union {
        int fd;                   /**< descriptor for file objects */
        void *socket;             /**< socket for ZeroMQ items */
        uint64_t timeout_ns;      /**< timeout (in ns) for timer objects */
    };
    union {
        int socket_fd;
        int timeout_fd;
        uint32_t timeout_id;
    };                            /**< internal management data */
    int type;                     /**< internal type */
};

typedef int (op_event_callback)(const struct op_event_data *data, void *arg);

/**
 * Event callback structure
 */
struct op_event_callbacks {
    union {
        struct {
            op_event_callback *on_read;     /**< called on read event */
            op_event_callback *on_write;    /**< called on write event */
            op_event_callback *on_close;    /**< called on close event */
        };                                   /**< used by file and socket events */
        struct {
            op_event_callback *on_timeout;  /**< called on timer expiration,
                                                  timer events only */
        };
    };
    op_event_callback *on_delete;           /**< called when event is deleted */
};

#endif /* _OP_EVENT_H_ */
