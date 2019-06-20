#ifndef _ICP_PACKETIO_STACK_TCPIP_HANDLERS_H_
#define _ICP_PACKETIO_STACK_TCPIP_HANDLERS_H_

#include <chrono>

#include "lwip/sys.h"

namespace icp::packetio::tcpip {

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

#endif /* _ICP_PACKETIO_STACK_TCPIP_HANDLERS_H_ */
