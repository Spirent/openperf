/**
 * @file
 * Dummy functions required by the stack but expected to be implemented
 * elsewhere, e.g. in the sockets module.
 */

#include "lwip/tcp.h"

err_t __attribute__((weak))
lwip_tcp_event(void* arg __attribute__((unused)),
               struct tcp_pcb* pcb __attribute__((unused)),
               enum lwip_event event __attribute__((unused)),
               struct pbuf* p __attribute__((unused)),
               uint16_t size __attribute__((unused)),
               err_t err __attribute__((unused)))
{
    return (ERR_OK);
}
