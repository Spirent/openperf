/**
 * @file
 * Sequential API Main thread module
 *
 */

/*
 * Copyright (c) 2001-2004 Swedish Institute of Computer Science.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
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
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT
 * SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
 * OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY
 * OF SUCH DAMAGE.
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

#include "core/icp_core.h"
#include "socket/server/api_server.h"

#include "packetio/drivers/dpdk/dpdk.h"

#define TCPIP_MSG_VAR_REF(name)     API_VAR_REF(name)
#define TCPIP_MSG_VAR_DECLARE(name) API_VAR_DECLARE(struct tcpip_msg, name)
#define TCPIP_MSG_VAR_ALLOC(name)   API_VAR_ALLOC(struct tcpip_msg, MEMP_TCPIP_MSG_API, name, ERR_MEM)
#define TCPIP_MSG_VAR_FREE(name)    API_VAR_FREE(MEMP_TCPIP_MSG_API, name)

#define TCPIP_MSG_SHUTDOWN  (TCPIP_MSG_CALLBACK_STATIC + 1)

/* global variables */
static tcpip_init_done_fn tcpip_init_done;
static void *tcpip_init_done_arg;
static sys_mbox_t tcpip_mbox;

static bool tcpip_shutting_down;

#if LWIP_TCPIP_CORE_LOCKING
/** The global semaphore to lock the stack. */
sys_mutex_t lock_tcpip_core;
#endif /* LWIP_TCPIP_CORE_LOCKING */

#if LWIP_TIMERS
/* The timeout_id for our timout callback */
static uint32_t tcpip_timeout_id;

/**
 * The icp event loop code expects timeouts to be specified as a uint64_t
 * containing nanosecond units.  Provide a convenience mechanism to perform
 * the conversion.
 */
using loop_ns = std::chrono::duration<uint64_t, std::nano>;

static int
handle_tcpip_timeout(const struct icp_event_data *data,
                     void *arg __attribute__((unused)))
{
    std::chrono::milliseconds sleeptime;
    do {
        sys_check_timeouts();
        sleeptime = std::chrono::milliseconds(sys_timeouts_sleeptime());
    } while (sleeptime.count() == 0);

    icp_event_loop_update(data->loop, tcpip_timeout_id,
                          std::chrono::duration_cast<loop_ns>(sleeptime).count());

    return (0);
}
#endif /* LWIP_TIMERS */

static int
handle_tcpip_msg(const struct icp_event_data *data, void *arg)
{
    LWIP_UNUSED_ARG(data);
    auto mbox = reinterpret_cast<sys_mbox_t>(arg);
    struct tcpip_msg *msg = nullptr;

    /* acknowledge notifications */
    sys_mbox_clear_notifications(&mbox);

    while (sys_arch_mbox_tryfetch(&mbox, (void **)&msg) != SYS_MBOX_EMPTY) {
        LOCK_TCPIP_CORE();
        if (msg == nullptr) {
            LWIP_DEBUGF(TCPIP_DEBUG, ("tcpip_thread: invalid message: NULL\n"));
            LWIP_ASSERT("tcpip_thread: invalid message", 0);
            continue;
        }

        switch (static_cast<int>(msg->type)) {
#if !LWIP_TCPIP_CORE_LOCKING
        case TCPIP_MSG_API:
            LWIP_DEBUGF(TCPIP_DEBUG, ("tcpip_thread: API message %p\n", (void *)msg));
            msg->msg.api_msg.function(msg->msg.api_msg.msg);
            break;
        case TCPIP_MSG_API_CALL:
            LWIP_DEBUGF(TCPIP_DEBUG, ("tcpip_thread: API CALL message %p\n", (void *)msg));
            msg->msg.api_call.arg->err = msg->msg.api_call.function(msg->msg.api_call.arg);
            sys_sem_signal(msg->msg.api_call.sem);
            break;
#endif /* !LWIP_TCPIP_CORE_LOCKING */

#if !LWIP_TCPIP_CORE_LOCKING_INPUT
        case TCPIP_MSG_INPKT:
            LWIP_DEBUGF(TCPIP_DEBUG, ("tcpip_thread: PACKET %p\n", (void *)msg));
            msg->msg.inp.input_fn(msg->msg.inp.p, msg->msg.inp.netif);
            memp_free(MEMP_TCPIP_MSG_INPKT, msg);
            break;
#endif /* !LWIP_TCPIP_CORE_LOCKING_INPUT */

#if LWIP_TCPIP_TIMEOUT && LWIP_TIMERS
        case TCPIP_MSG_TIMEOUT:
            LWIP_DEBUGF(TCPIP_DEBUG, ("tcpip_thread: TIMEOUT %p\n", (void *)msg));
            sys_timeout(msg->msg.tmo.msecs, msg->msg.tmo.h, msg->msg.tmo.arg);
            memp_free(MEMP_TCPIP_MSG_API, msg);
            break;
        case TCPIP_MSG_UNTIMEOUT:
            LWIP_DEBUGF(TCPIP_DEBUG, ("tcpip_thread: UNTIMEOUT %p\n", (void *)msg));
            sys_untimeout(msg->msg.tmo.h, msg->msg.tmo.arg);
            memp_free(MEMP_TCPIP_MSG_API, msg);
            break;
#endif /* LWIP_TCPIP_TIMEOUT && LWIP_TIMERS */

        case TCPIP_MSG_CALLBACK:
            LWIP_DEBUGF(TCPIP_DEBUG, ("tcpip_thread: CALLBACK %p\n", (void *)msg));
            msg->msg.cb.function(msg->msg.cb.ctx);
            memp_free(MEMP_TCPIP_MSG_API, msg);
            break;

        case TCPIP_MSG_CALLBACK_STATIC:
            LWIP_DEBUGF(TCPIP_DEBUG, ("tcpip_thread: CALLBACK_STATIC %p\n", (void *)msg));
            msg->msg.cb.function(msg->msg.cb.ctx);
            break;

        case TCPIP_MSG_SHUTDOWN:
            icp_event_loop_exit(data->loop);
            sys_sem_signal(msg->msg.api_call.sem);
            tcpip_shutting_down = true;
            break;

        default:
            LWIP_DEBUGF(TCPIP_DEBUG, ("tcpip_thread: invalid message: %d\n", msg->type));
            LWIP_ASSERT("tcpip_thread: invalid message", 0);
            break;
        }

        LWIP_TCPIP_THREAD_ALIVE();
    }

    return (0);
}

extern "C" {

/**
 * The main lwIP thread. This thread has exclusive access to lwIP core functions
 * (unless access to them is not locked). Other threads communicate with this
 * thread using message boxes.
 *
 * It also starts all the timers to make sure they are running in the right
 * thread context.
 *
 * @param arg unused argument
 */
static void
tcpip_thread(void *arg)
{
    LWIP_UNUSED_ARG(arg);

    icp::core::event_loop tcpip_loop;

    icp::socket::api::server sock_serv(tcpip_loop);

    struct icp_event_callbacks msg_callbacks = {
        .on_read = handle_tcpip_msg,
    };

    tcpip_loop.add(sys_mbox_fd(&tcpip_mbox), &msg_callbacks, tcpip_mbox);

#if LWIP_TIMERS
    struct icp_event_callbacks timer_callbacks = {
        .on_read = handle_tcpip_timeout,
    };

    auto sleeptime = std::chrono::milliseconds(sys_timeouts_sleeptime());
    tcpip_loop.add(std::chrono::duration_cast<loop_ns>(sleeptime).count(),
             &timer_callbacks, nullptr, &tcpip_timeout_id);
#endif

    if (tcpip_init_done != nullptr) {
        tcpip_init_done(tcpip_init_done_arg);
    }

    extern int eal_workers_wrapper();
    extern void worker_proxy(bool);

    if (eal_workers_wrapper() > 1) {
        tcpip_loop.run();
    } else {
        while (true) {
            tcpip_loop.run(0);
            worker_proxy(tcpip_shutting_down);
            if (tcpip_shutting_down) break;
        }
    }
    sys_mbox_free(&tcpip_mbox);
}

/**
 * Pass a received packet to tcpip_thread for input processing
 *
 * @param p the received packet
 * @param inp the network interface on which the packet was received
 * @param input_fn input function to call
 */
err_t
tcpip_inpkt(struct pbuf *p, struct netif *inp, netif_input_fn input_fn)
{
#if LWIP_TCPIP_CORE_LOCKING_INPUT
    err_t ret;
    LWIP_DEBUGF(TCPIP_DEBUG, ("tcpip_inpkt: PACKET %p/%p\n", (void *)p, (void *)inp));
    LOCK_TCPIP_CORE();
    ret = input_fn(p, inp);
    UNLOCK_TCPIP_CORE();
    return ret;
#else /* LWIP_TCPIP_CORE_LOCKING_INPUT */
    struct tcpip_msg *msg;

    LWIP_ASSERT("Invalid mbox", sys_mbox_valid_val(tcpip_mbox));

    msg = (struct tcpip_msg *)memp_malloc(MEMP_TCPIP_MSG_INPKT);
    if (msg == nullptr) {
        return ERR_MEM;
    }

    msg->type = TCPIP_MSG_INPKT;
    msg->msg.inp.p = p;
    msg->msg.inp.netif = inp;
    msg->msg.inp.input_fn = input_fn;
    if (sys_mbox_trypost(&tcpip_mbox, msg) != ERR_OK) {
        memp_free(MEMP_TCPIP_MSG_INPKT, msg);
        return ERR_MEM;
    }
    return ERR_OK;
#endif /* LWIP_TCPIP_CORE_LOCKING_INPUT */
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
err_t
tcpip_input(struct pbuf *p, struct netif *inp)
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
err_t
tcpip_callback(tcpip_callback_fn function, void *ctx)
{
    struct tcpip_msg *msg;

    LWIP_ASSERT("Invalid mbox", sys_mbox_valid_val(tcpip_mbox));

    msg = (struct tcpip_msg *)memp_malloc(MEMP_TCPIP_MSG_API);
    if (msg == NULL) {
        return ERR_MEM;
    }

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
err_t
tcpip_try_callback(tcpip_callback_fn function, void *ctx)
{
    struct tcpip_msg *msg;

    LWIP_ASSERT("Invalid mbox", sys_mbox_valid_val(tcpip_mbox));

    msg = (struct tcpip_msg *)memp_malloc(MEMP_TCPIP_MSG_API);
    if (msg == NULL) {
        return ERR_MEM;
    }

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
err_t
tcpip_api_call(tcpip_api_call_fn fn, struct tcpip_api_call_data *call)
{
    /*
     * XXX: Shutdown can cause a race on this value, so don't proceed if we've
     * already destroyed the tcpip_mbox.
     */
    if (!sys_mbox_valid_val(tcpip_mbox)) {
        return (-1);
    }

#if LWIP_TCPIP_CORE_LOCKING
    err_t err;
    LOCK_TCPIP_CORE();
    err = fn(call);
    UNLOCK_TCPIP_CORE();
    return err;
#else /* LWIP_TCPIP_CORE_LOCKING */
    TCPIP_MSG_VAR_DECLARE(msg);

#if !LWIP_NETCONN_SEM_PER_THREAD
    err_t err = sys_sem_new(&call->sem, 0);
    if (err != ERR_OK) {
        return err;
    }
#endif /* LWIP_NETCONN_SEM_PER_THREAD */

    TCPIP_MSG_VAR_ALLOC(msg);
    TCPIP_MSG_VAR_REF(msg).type = TCPIP_MSG_API_CALL;
    TCPIP_MSG_VAR_REF(msg).msg.api_call.arg = call;
    TCPIP_MSG_VAR_REF(msg).msg.api_call.function = fn;
#if LWIP_NETCONN_SEM_PER_THREAD
    TCPIP_MSG_VAR_REF(msg).msg.api_call.sem = LWIP_NETCONN_THREAD_SEM_GET();
#else /* LWIP_NETCONN_SEM_PER_THREAD */
    TCPIP_MSG_VAR_REF(msg).msg.api_call.sem = &call->sem;
#endif /* LWIP_NETCONN_SEM_PER_THREAD */
    sys_mbox_post(&tcpip_mbox, &TCPIP_MSG_VAR_REF(msg));
    sys_arch_sem_wait(TCPIP_MSG_VAR_REF(msg).msg.api_call.sem, 0);
    TCPIP_MSG_VAR_FREE(msg);

#if !LWIP_NETCONN_SEM_PER_THREAD
    sys_sem_free(&call->sem);
#endif /* LWIP_NETCONN_SEM_PER_THREAD */

    return call->err;
#endif /* LWIP_TCPIP_CORE_LOCKING */
}

/**
 * @ingroup lwip_os
 * Initialize this module:
 * - initialize all sub modules
 * - start the tcpip_thread
 *
 * @param initfunc a function to call when tcpip_thread is running and finished initializing
 * @param arg argument to pass to initfunc
 */
void
tcpip_init(tcpip_init_done_fn initfunc, void *arg)
{
    lwip_init();

    tcpip_init_done = initfunc;
    tcpip_init_done_arg = arg;

    if (sys_mbox_new(&tcpip_mbox, TCPIP_MBOX_SIZE) != ERR_OK) {
        LWIP_ASSERT("failed to create tcpip_thread mbox", 0);
    }
#if LWIP_TCPIP_CORE_LOCKING
    if (sys_mutex_new(&lock_tcpip_core) != ERR_OK) {
        LWIP_ASSERT("failed to create lock_tcpip_core", 0);
    }
#endif /* LWIP_TCPIP_CORE_LOCKING */

    sys_thread_new(TCPIP_THREAD_NAME, tcpip_thread, nullptr, TCPIP_THREAD_STACKSIZE, TCPIP_THREAD_PRIO);
}

err_t
tcpip_shutdown()
{
    if (!sys_mbox_valid_val(tcpip_mbox)) {
        return (ERR_CLSD);
    }

#if LWIP_TCPIP_CORE_LOCKING
    return ERR_VAL;
#else /* LWIP_TCPIP_CORE_LOCKING */
    TCPIP_MSG_VAR_DECLARE(msg);

#if !LWIP_NETCONN_SEM_PER_THREAD
    sys_sem_t sem;
    err_t err = sys_sem_new(&sem, 0);
    if (err != ERR_OK) {
        return err;
    }
#endif /* LWIP_NETCONN_SEM_PER_THREAD */
    TCPIP_MSG_VAR_ALLOC(msg);
    TCPIP_MSG_VAR_REF(msg).type = static_cast<tcpip_msg_type>(TCPIP_MSG_SHUTDOWN);
#if LWIP_NETCONN_SEM_PER_THREAD
    TCPIP_MSG_VAR_REF(msg).msg.api_call.sem = LWIP_NETCONN_THREAD_SEM_GET();
#else
    TCPIP_MSG_VAR_REF(msg).msg.api_call.sem = &sem;
#endif
    sys_mbox_post(&tcpip_mbox, &TCPIP_MSG_VAR_REF(msg));
    sys_arch_sem_wait(TCPIP_MSG_VAR_REF(msg).msg.api_call.sem, 0);
    TCPIP_MSG_VAR_FREE(msg);

#if !LWIP_NETCONN_SEM_PER_THREAD
    sys_sem_free(&sem);
#endif /* LWIP_NETCONN_SEM_PER_THREAD */

    return (ERR_OK);
#endif /* LWIP_TCPIP_CORE_LOCKING */
}

}

#endif /* !NO_SYS */
