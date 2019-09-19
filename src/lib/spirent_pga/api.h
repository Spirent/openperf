#ifndef _LIB_SPIRENT_PGA_API_H_
#define _LIB_SPIRENT_PGA_API_H_

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

/**
 * @file
 *
 * This file contains the API for the Spirent PGA library.
 */

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Initialize the library.
 *
 * This is the only API function which can allocate memory and does so
 * in order to benchmark the internal function implementations.  All
 * allocated memory is released before returning.
 *
 * Additionally, this function is idempotent, but only performs the
 * internal initialization once.  Subsequent calls to this function are
 * effectively no-ops.
 *
 * Finally, calling this function is not strictly necessary as the library
 * will automatically initialize itself when needed.  However, it is provided
 * as a convenience so that clients can perform the benchmarking routine
 * at program start-up.
 */
void pga_init();

/**
 * Log library details to the specified file.
 *
 * @param[in] output
 *   stream to write data to
 */
void pga_log_info(FILE* output);

/**
 * Calculate the non-zero checksum of (payload & mask)
 * This function is necessary to fill in the Spirent signature checksum
 * cheater field when using 64 & 65 byte UDP test frames.
 *7
 * @param[in] payload
 *   pointer to data to checksum
 * @param[in] mask
 *   pointer to mask indicating which bits to include in checksum calculation
 * @param[in] length of payload
 *
 * @return
 *   non-zero 16 bit checksum
 */
uint16_t pga_checksum_udp(const uint8_t payload[], const uint8_t mask[], uint16_t length);

/**
 * These enums define specific signature bit field values.
 */
enum pga_signature_prbs {
    enable,   /**< Payload contains PRBS data */
    disable   /**< Payload does not contain PRBS data */
};

enum pga_signature_timestamp {
    first,  /**< Timestamp refers to the first bit of the containing frame */
    last    /**< Timestamp refers to the last bit of the containing frame */
};

/**
 * Retrieve the signature PRBS value from the specified flag
 *
 * @param[in] flag
 *   signature flag value as used by encode/decode functions
 *
 * @return
 *   PRBS value of flag
 */
enum pga_signature_prbs pga_prbs_flag(int flag);

/**
 * Retrieve the signature timestamp value from the specified flag
 *
 * @param[in] flag
 *   signature flag value as used by the encode/decode functions
 *
 * @return
 *   timestamp value of flag
 */
enum pga_signature_timestamp pga_timestamp_flag(int flag);

/**
 * Generate a signature flag value from specified PRBS and timestamp settings.
 *
 * @param[in] prbs
 *   signature PRBS setting
 * @param[in] ts
 *   signature timestamp setting
 *
 * @return
 *  signature flag value as used by the encode/decode functions
 */
int pga_signature_flag(enum pga_signature_prbs prbs,
                       enum pga_signature_timestamp ts);

/**
 * Attempt to decode Spirent signatures.  Signature data is written to the output
 * arrays sequentially.  That is, if there are m payloads, but only n signatures
 * detected, where n < m, then the output arrays will contain data in indexes
 * [0, n).
 * Note that the output arrays should be at least as large as the payloads array.
 *
 * @param[in] payloads
 *   array of pointers to suspected signature data
 * @param[in] count
 *   the length of the payloads array
 * @param[out] stream_ids
 *   stream ids for decoded signatures
 * @param[out] sequence_numbers
 *   sequence numbers for decoded signatures
 * @param[out] tx_timestamps
 *   transmit timestamps for decoded signatures
 * @param[out] flags
 *   flags for decoded signatures
 *
 * @return
 *   The number of decode signatures.
 */
uint16_t pga_signatures_decode(const uint8_t* payloads[], uint16_t count,
                               uint32_t stream_ids[],
                               uint32_t sequence_numbers[],
                               uint64_t tx_timestamps[],
                               int flags[]);

/**
 * Encode Spirent signature data into the destinations array.
 *
 * @param[in] destinations
 *   array containing pointers to location where signatures should be written;
 *   each pointer should contain a buffer with at room for at least 20 bytes
 * @param[in] stream_ids
 *   array containing stream ids for each signature to write
 * @param[in] sequence_numbers
 *   array containing sequence numbers for each signature to write
 * @param[in] timestamp
 *   tx timestamp for this burst of signatures; since no two signatures should
 *   have the same transmit timestamp each encoded signature will increase this
 *   value by 1
 * @param[in] flags
 *   flags value for all generated signatures
 * @param[in] count
 *   the number of signatures to write
 */
void pga_signatures_encode(uint8_t* destinations[],
                           const uint32_t stream_ids[],
                           const uint32_t sequence_numbers[],
                           uint64_t timestamp, int flags, uint16_t count);

/**
 * Write constant octets to payloads
 *
 * @param[in] payloads
 *   array of pointers to write fill data to
 * @param[in] lengths
 *   the number of octets to write to each payload pointer
 * @param[in] count
 *   the number of payloads
 * @param[in] base
 *   the value to write to payloads
 */
void pga_fill_const(uint8_t* payloads[], uint16_t lengths[], uint16_t count, uint8_t base);

/**
 * Write decrementing octets to payloads.  Each subsequent octet written to
 * the payload will be decremented by 1.
 *
 * @param[in] payloads
 *   array of pointers to write fill data to
 * @param[in] lengths
 *   the number of bytes to write to each payload pointer
 * @param[in] count
 *   the number of payloads
 * @param[in] base
 *   the initial value to write to each payload, e.g. the value of payloads[i][0]
 */
void  pga_fill_decr(uint8_t* payloads[], uint16_t lengths[], uint16_t count, uint8_t base);

/**
 * Write incrementing octets to paylods.  Each subsequent octet written to
 * the payload will be incremented by 1.
 *
 * @param[in] payloads
 *   array of pointers to write fill data to
 * @param[in] lengths
 *   the number of octets to write to each payload pointer
 * @param[in] count
 *   the number of payloads
 * @param[in] base
 *   the initial value to write to each payload, e.g. the value of payloads[i][0]
 */
void  pga_fill_incr(uint8_t* payloads[], uint16_t lengths[], uint16_t count, uint8_t base);

/**
 * Write PRBS data to payloads.  The PRBS sequence will continue across payloads.
 * The returned value may be used continue the sequence to a new set of payloads.
 *
 * @param[in] payloads
 *   array of pointers to write fill data to
 * @param[in] lengths
 *   the number of octets to write to each payload pointer
 * @param[in] count
 *   the number of payloads
 * @param[in] seed
 *   the seed value for the PRBS generator
 *
 * @return
 *   the seed value for the next 4 octets of data
 */
uint32_t pga_fill_prbs(uint8_t* payloads[], uint16_t lengths[], uint16_t count, uint32_t seed);

/**
 * Verify PRBS data pointed to by payloads.
 *
 * @param[in] payloads
 *   array of pointers containing PRBS payload data
 * @param[in] lengths
 *   the number of octets to verify for each payload pointer
 * @param[in] count
 *   the number of payloads
 * @param[out] bit_errors
 *   bit error counts for each payload; array must be as long as payloads
 *
 * @return
 *   - true: bit errors detected; see bit_errors array for details
 *   - false: no bit errors detected
 */
bool pga_verify_prbs(uint8_t* payloads[], uint16_t lengths[], uint16_t count,
                     uint32_t bit_errors[]);

/**
 * Generate checksums for IPv4 headers.
 *
 * @param[in] ipv4_headers
 *   array of pointers to IPv4 headers
 * @param[in] count
 *   the number of headers in the array
 * @param[out] checksums
 *   output array of checksums; checksum[i] contains the checksum of the i'th
 *   header
 */
void pga_checksum_ipv4_headers(const uint8_t* ipv4_headers[], uint16_t count,
                               uint32_t checksums[]);

/**
 * Generate TCP/UDP checksums.
 *
 * @param[in] ipv4_headers
 *   array of pointers to IPv4 headers.  The layer 4 data to checksum should
 *   follow the IPv4 header
 * @param[in] count
 *   the number of headers/payloads to checksum
 * @param[out] checksums
 *   output array of checksums; checksum[i] contains the checksum for the i'th
 *   header/payload
 */
void pga_checksum_ipv4_tcpudp(const uint8_t* ipv4_headers[], uint16_t count,
                              uint32_t checksums[]);

#ifdef __cplusplus
}
#endif

#endif /* _LIB_SPIRENT_PGA_API_H_ */
