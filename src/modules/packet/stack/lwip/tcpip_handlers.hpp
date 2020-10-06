#ifndef _OP_PACKET_STACK_TCPIP_HANDLERS_HPP_
#define _OP_PACKET_STACK_TCPIP_HANDLERS_HPP_

#include <chrono>

#include "lwip/sys.h"

/*
 * LwIP global variable needed to send messages between clients
 * and the stack context.
 */
extern sys_mbox_t tcpip_mbox;

namespace openperf::packetio::stack::tcpip {

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

} // namespace openperf::packet::stack::tcpip

#endif /* _OP_PACKET_STACK_TCPIP_HANDLERS_HPP_ */
