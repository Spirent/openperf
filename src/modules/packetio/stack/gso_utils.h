#ifndef _OP_PACKETIO_STACK_GSO_UTILS_H_
#define _OP_PACKETIO_STACK_GSO_UTILS_H_

#include <stdbool.h>
#include <stdint.h>

struct pbuf;
struct tcp_pcb;
struct tcp_seg;

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Calculate the amount of extra space in a pbuf chain of the specified
 * length.  Since pbuf/mbufs are allocated as fixed size blocks, there is
 * extra space available if length is not a multiple of the block size.
 */
uint16_t packetio_stack_gso_oversize_calc_length(uint16_t length);

/**
 * Figure out the maximum segment length this pcb can transmit.
 * The pcb is used to determine the outgoing interface, which is
 * then queried for the maximum segment length.
 */
uint16_t packetio_stack_gso_max_segment_length(const struct tcp_pcb*);

/**
 * Drop leading (acked) data from the specified segment.  This is called by
 * tcp_free_acked_segments() to trim data from partially acknowledged segments.
 *
 * Returns the number of pbufs freed from segment's chain so the pcb's
 * snd_queuelen can be updated.  Since no allocations are necessary, this
 * function should never fail.
 */
uint16_t packetio_stack_gso_segment_ack_partial(struct tcp_seg* seg,
                                                uint16_t acked);

/**
 * Split a segment at the specified split value.
 * Called by tcp_output() when the segment to transmit is larger than the
 * send window.
 *
 * On success, the new segment is appended to the given segment, and the pcb
 * is updated to reflect the change.
 * On failure, an error code is returned and the segment and pcb are unmodified.
 */
int packetio_stack_gso_segment_split(struct tcp_pcb* pcb,
                                     struct tcp_seg* seg,
                                     uint16_t split);

/**
 * Returns the amount of unused payload in the pbuf after the given offset.
 */
uint16_t packetio_stack_gso_pbuf_data_available(const struct pbuf* p,
                                                uint16_t offset);

/**
 * Copy data into a pbuf chain, starting at the specified offset.
 */
uint16_t packetio_stack_gso_pbuf_copy(struct pbuf* p_head,
                                      uint16_t offset,
                                      const void* dataptr,
                                      uint16_t length);

#ifdef __cplusplus
}
#endif

#endif /* _OP_PACKETIO_STACK_GSO_UTILS_H_ */
