#ifndef _OP_PACKETIO_STACK_TCPIP_HPP_
#define _OP_PACKETIO_STACK_TCPIP_HPP_

#include <chrono>

#include "lwip/sys.h"

/*
 * LwIP global variable needed to send messages between clients
 * and the stack context.
 */

namespace openperf::packetio::tcpip {

/*
 * Unfortunately, lwIP uses a global mailbox to move messages between
 * clients and the stack thread.  This function provides access to it.
 */
sys_mbox_t mbox();

/**
 * Process all pending timeouts in the stack
 * Returns the number of milliseconds utill the next timeout
 */
std::chrono::milliseconds handle_timeouts();

/**
 * Handle all pending stack messages in the mbox.
 * Clears all notifications from the mbox before returning.
 * Returns -1 on error or if a shutdown message is received.
 */
int handle_messages(sys_mbox_t);

}

#endif /* _OP_PACKETIO_STACK_TCPIP_HPP_ */
