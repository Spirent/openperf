/**
 * @file
 * Sequential API Main thread module
 *
 */

/*
 * Copyright (c) 2001-2004 Swedish Institute of Computer Science.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
 * EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * This file is part of the lwIP TCP/IP stack.
 *
 * Author: Adam Dunkels <adam@sics.se>
 *
 */

#include <cstdint>
#include <cstdlib>
#include <chrono>
#include <ratio>

#include "lwip/opt.h"

#if !NO_SYS /* don't build if not configured for use in lwipopts.h */

#include "lwip/priv/tcpip_priv.h"
#include "lwip/sys.h"
#include "lwip/memp.h"
#include "lwip/mem.h"
#include "lwip/init.h"
#include "lwip/ip.h"
#include "lwip/pbuf.h"
#include "lwip/etharp.h"
#include "netif/ethernet.h"

#include "core/op_core.h"
#include "packetio/stack/tcpip.hpp"

#define TCPIP_MSG_VAR_REF(name) API_VAR_REF(name)
#define TCPIP_MSG_VAR_DECLARE(name) API_VAR_DECLARE(struct tcpip_msg, name)
#define TCPIP_MSG_VAR_ALLOC(name)                                              \
    API_VAR_ALLOC(struct tcpip_msg, MEMP_TCPIP_MSG_API, name, ERR_MEM)
#define TCPIP_MSG_VAR_FREE(name) API_VAR_FREE(MEMP_TCPIP_MSG_API, name)

#if LWIP_TCPIP_CORE_LOCKING
#error "lwip's core locking is not supported"
#endif /* LWIP_TCPIP_CORE_LOCKING */

namespace openperf::packetio::tcpip {

/**
 * These next two functions, handle_timeouts() and handle_messages() provide
 * the core functionality of the lwip TCP/IP stack.
 */

std::chrono::milliseconds handle_timeouts()
{
    std::chrono::milliseconds sleeptime;
    do {
        sys_check_timeouts();
        sleeptime = std::chrono::milliseconds(sys_timeouts_sleeptime());
    } while (sleeptime.count() == 0);

    return (sleeptime);
}

int handle_messages(sys_mbox_t mbox)
{
    struct tcpip_msg* msg = nullptr;

    LWIP_TCPIP_THREAD_ALIVE();

    /* acknowledge notifications */
    sys_mbox_clear_notifications(&mbox);

    while (sys_arch_mbox_tryfetch(&mbox, (void**)&msg) != SYS_MBOX_EMPTY) {
        if (msg == nullptr) {
            LWIP_DEBUGF(TCPIP_DEBUG, ("tcpip: invalid message: NULL\n"));
            LWIP_ASSERT("tcpip: invalid message", 0);
            continue;
        }

        switch (static_cast<int>(msg->type)) {
        case TCPIP_MSG_API:
            LWIP_DEBUGF(TCPIP_DEBUG, ("tcpip: API message %p\n", (void*)msg));
            msg->msg.api_msg.function(msg->msg.api_msg.msg);
            break;

        case TCPIP_MSG_API_CALL:
            LWIP_DEBUGF(TCPIP_DEBUG,
                        ("tcpip: API CALL message %p\n", (void*)msg));
            msg->msg.api_call.arg->err =
                msg->msg.api_call.function(msg->msg.api_call.arg);
            sys_sem_signal(msg->msg.api_call.sem);
            break;

        case TCPIP_MSG_INPKT:
            LWIP_DEBUGF(TCPIP_DEBUG, ("tcpip: PACKET %p\n", (void*)msg));
            msg->msg.inp.input_fn(msg->msg.inp.p, msg->msg.inp.netif);
            memp_free(MEMP_TCPIP_MSG_INPKT, msg);
            break;

        case TCPIP_MSG_CALLBACK:
            LWIP_DEBUGF(TCPIP_DEBUG, ("tcpip: CALLBACK %p\n", (void*)msg));
            msg->msg.cb.function(msg->msg.cb.ctx);
            memp_free(MEMP_TCPIP_MSG_API, msg);
            break;

        case TCPIP_MSG_CALLBACK_STATIC:
            LWIP_DEBUGF(TCPIP_DEBUG,
                        ("tcpip: CALLBACK_STATIC %p\n", (void*)msg));
            msg->msg.cb.function(msg->msg.cb.ctx);
            break;

        default:
            LWIP_DEBUGF(TCPIP_DEBUG,
                        ("tcpip: invalid message: %d\n", msg->type));
            LWIP_ASSERT("tcpip: invalid message", 0);
            break;
        }
    }

    return (0);
}

} // namespace openperf::packetio::tcpip

extern "C" {

/**
 * Pass a received packet to tcpip_thread for input processing
 *
 * @param p the received packet
 * @param inp the network interface on which the packet was received
 * @param input_fn input function to call
 */
err_t tcpip_inpkt(struct pbuf* p, struct netif* inp, netif_input_fn input_fn)
{
    auto tcpip_mbox = openperf::packetio::tcpip::mbox();
    struct tcpip_msg* msg;

    LWIP_ASSERT("Invalid mbox", sys_mbox_valid_val(tcpip_mbox));

    msg = (struct tcpip_msg*)memp_malloc(MEMP_TCPIP_MSG_INPKT);
    if (msg == nullptr) { return ERR_MEM; }

    msg->type = TCPIP_MSG_INPKT;
    msg->msg.inp.p = p;
    msg->msg.inp.netif = inp;
    msg->msg.inp.input_fn = input_fn;
    if (sys_mbox_trypost(&tcpip_mbox, msg) != ERR_OK) {
        memp_free(MEMP_TCPIP_MSG_INPKT, msg);
        return ERR_MEM;
    }
    return ERR_OK;
}

/**
 * @ingroup lwip_os
 * Pass a received packet to tcpip_thread for input processing with
 * ethernet_input or ip_input. Don't call directly, pass to netif_add()
 * and call netif->input().
 *
 * @param p the received packet, p->payload pointing to the Ethernet header or
 *          to an IP header (if inp doesn't have NETIF_FLAG_ETHARP or
 *          NETIF_FLAG_ETHERNET flags)
 * @param inp the network interface on which the packet was received
 */
err_t tcpip_input(struct pbuf* p, struct netif* inp)
{
#if LWIP_ETHERNET
    if (inp->flags & (NETIF_FLAG_ETHARP | NETIF_FLAG_ETHERNET)) {
        return tcpip_inpkt(p, inp, ethernet_input);
    } else
#endif /* LWIP_ETHERNET */
        return tcpip_inpkt(p, inp, ip_input);
}

/**
 * @ingroup lwip_os
 * Call a specific function in the thread context of
 * tcpip_thread for easy access synchronization.
 * A function called in that way may access lwIP core code
 * without fearing concurrent access.
 * Blocks until the request is posted.
 * Must not be called from interrupt context!
 *
 * @param function the function to call
 * @param ctx parameter passed to f
 * @return ERR_OK if the function was called, another err_t if not
 *
 * @see tcpip_try_callback
 */
err_t tcpip_callback(tcpip_callback_fn function, void* ctx)
{
    auto tcpip_mbox = openperf::packetio::tcpip::mbox();
    struct tcpip_msg* msg;

    LWIP_ASSERT("Invalid mbox", sys_mbox_valid_val(tcpip_mbox));

    msg = (struct tcpip_msg*)memp_malloc(MEMP_TCPIP_MSG_API);
    if (msg == nullptr) { return ERR_MEM; }

    msg->type = TCPIP_MSG_CALLBACK;
    msg->msg.cb.function = function;
    msg->msg.cb.ctx = ctx;

    sys_mbox_post(&tcpip_mbox, msg);
    return ERR_OK;
}

/**
 * @ingroup lwip_os
 * Call a specific function in the thread context of
 * tcpip_thread for easy access synchronization.
 * A function called in that way may access lwIP core code
 * without fearing concurrent access.
 * Does NOT block when the request cannot be posted because the
 * tcpip_mbox is full, but returns ERR_MEM instead.
 * Can be called from interrupt context.
 *
 * @param function the function to call
 * @param ctx parameter passed to f
 * @return ERR_OK if the function was called, another err_t if not
 *
 * @see tcpip_callback
 */
err_t tcpip_try_callback(tcpip_callback_fn function, void* ctx)
{
    auto tcpip_mbox = openperf::packetio::tcpip::mbox();
    struct tcpip_msg* msg;

    LWIP_ASSERT("Invalid mbox", sys_mbox_valid_val(tcpip_mbox));

    msg = (struct tcpip_msg*)memp_malloc(MEMP_TCPIP_MSG_API);
    if (msg == nullptr) { return ERR_MEM; }

    msg->type = TCPIP_MSG_CALLBACK;
    msg->msg.cb.function = function;
    msg->msg.cb.ctx = ctx;

    if (sys_mbox_trypost(&tcpip_mbox, msg) != ERR_OK) {
        memp_free(MEMP_TCPIP_MSG_API, msg);
        return ERR_MEM;
    }
    return ERR_OK;
}

/**
 * Synchronously calls function in TCPIP thread and waits for its completion.
 * It is recommended to use LWIP_TCPIP_CORE_LOCKING (preferred) or
 * LWIP_NETCONN_SEM_PER_THREAD.
 * If not, a semaphore is created and destroyed on every call which is usually
 * an expensive/slow operation.
 * @param fn Function to call
 * @param call Call parameters
 * @return Return value from tcpip_api_call_fn
 */
err_t tcpip_api_call(tcpip_api_call_fn fn, struct tcpip_api_call_data* call)
{
    auto tcpip_mbox = openperf::packetio::tcpip::mbox();
    /*
     * XXX: Shutdown can cause a race on this value, so don't proceed if we've
     * already destroyed the tcpip_mbox.
     */
    if (!sys_mbox_valid_val(tcpip_mbox)) { return (-1); }

    TCPIP_MSG_VAR_DECLARE(msg);

#if !LWIP_NETCONN_SEM_PER_THREAD
    err_t err = sys_sem_new(&call->sem, 0);
    if (err != ERR_OK) { return err; }
#endif /* LWIP_NETCONN_SEM_PER_THREAD */

    TCPIP_MSG_VAR_ALLOC(msg);
    TCPIP_MSG_VAR_REF(msg).type = TCPIP_MSG_API_CALL;
    TCPIP_MSG_VAR_REF(msg).msg.api_call.arg = call;
    TCPIP_MSG_VAR_REF(msg).msg.api_call.function = fn;
#if LWIP_NETCONN_SEM_PER_THREAD
    TCPIP_MSG_VAR_REF(msg).msg.api_call.sem = LWIP_NETCONN_THREAD_SEM_GET();
#else  /* LWIP_NETCONN_SEM_PER_THREAD */
    TCPIP_MSG_VAR_REF(msg).msg.api_call.sem = &call->sem;
#endif /* LWIP_NETCONN_SEM_PER_THREAD */
    sys_mbox_post(&tcpip_mbox, &TCPIP_MSG_VAR_REF(msg));
    sys_arch_sem_wait(TCPIP_MSG_VAR_REF(msg).msg.api_call.sem, 0);
    TCPIP_MSG_VAR_FREE(msg);

#if !LWIP_NETCONN_SEM_PER_THREAD
    sys_sem_free(&call->sem);
#endif /* LWIP_NETCONN_SEM_PER_THREAD */

    return call->err;
}
}

#endif /* !NO_SYS */
