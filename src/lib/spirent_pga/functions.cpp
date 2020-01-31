#include <algorithm>
#include <array>
#include "functions.h"

namespace pga {

constexpr uint16_t nb_ipv4_headers = 32;
constexpr uint16_t ipv4_header_length = 20; /* octets */

constexpr uint16_t nb_signatures = 32;
constexpr uint16_t signature_length = 20;    /* octets */
constexpr uint16_t fill_buffer_length = 128; /* quadlets */

/***
 * Implementation function initialization routines.
 * Note: we intentionally use std::array<> so that library initialization
 * doesn't require any memory allocations.
 ***/

template <typename Tag>
void initialize_checksum_ipv4_headers(
    function_wrapper<checksum_ipv4_headers_fn, Tag>& wrapper)
{
    std::array<uint8_t[ipv4_header_length], nb_ipv4_headers> ipv4_headers;
    std::array<uint8_t*, nb_ipv4_headers> ipv4_header_ptrs;
    std::array<uint32_t, nb_ipv4_headers> checksums;

    std::transform(std::begin(ipv4_headers),
                   std::end(ipv4_headers),
                   std::begin(ipv4_header_ptrs),
                   [](auto& buffer) { return (std::addressof(buffer[0])); });

    wrapper.init((const uint8_t**)ipv4_header_ptrs.data(),
                 nb_ipv4_headers,
                 checksums.data());
}

void initialize_checksum_data(
    function_wrapper<checksum_data_aligned_fn>& wrapper)
{
    std::array<uint32_t, fill_buffer_length> buffer;
    scalar::fill_prbs_aligned(buffer.data(), buffer.size(), 0xffffffff);

    wrapper.init(buffer.data(), buffer.size());
}

void initialize_encode_signatures(
    function_wrapper<encode_signatures_fn>& wrapper)
{
    std::array<uint8_t[signature_length], nb_signatures> signatures;
    std::array<uint8_t*, nb_signatures> signature_ptrs;
    std::array<uint32_t, nb_signatures> stream_ids;
    std::array<uint32_t, nb_signatures> sequence_nums;

    for (auto i = 0; i < nb_signatures; i++) {
        signature_ptrs[i] = &signatures[i][0];
        stream_ids[i] = 0xc0ffee00 + i;
        sequence_nums[i] = i;
    }

    wrapper.init(signature_ptrs.data(),
                 stream_ids.data(),
                 sequence_nums.data(),
                 0xdeadbeef,
                 0x0,
                 nb_signatures);
}

void initialize_decode_signatures(
    function_wrapper<decode_signatures_fn>& wrapper)
{
    std::array<uint8_t[signature_length], nb_signatures> signatures;
    std::array<uint8_t*, nb_signatures> signature_ptrs;
    std::array<uint32_t, nb_signatures> stream_ids;
    std::array<uint32_t, nb_signatures> sequence_nums;

    for (auto i = 0; i < nb_signatures; i++) {
        signature_ptrs[i] = &signatures[i][0];
        stream_ids[i] = 0xc0ffee00 + i;
        sequence_nums[i] = i;
    }

    /*
     * We need some valid signatures to decode and the scalar functions
     * are always available.
     */
    scalar::encode_signatures(signature_ptrs.data(),
                              stream_ids.data(),
                              sequence_nums.data(),
                              0xdeadbeef,
                              0x0,
                              nb_signatures);

    std::array<uint32_t, nb_signatures> out_stream_ids;
    std::array<uint32_t, nb_signatures> out_sequence_nums;
    std::array<uint32_t, nb_signatures> out_timestamps_hi;
    std::array<uint32_t, nb_signatures> out_timestamps_lo;
    std::array<int, nb_signatures> out_flags;

    wrapper.init(signature_ptrs.data(),
                 nb_signatures,
                 out_stream_ids.data(),
                 out_sequence_nums.data(),
                 out_timestamps_hi.data(),
                 out_timestamps_lo.data(),
                 out_flags.data());
}

void initialize_fill_step(function_wrapper<fill_step_aligned_fn>& wrapper)
{
    std::array<uint32_t, fill_buffer_length> buffer;
    wrapper.init(buffer.data(), buffer.size(), 0x1, 1);
}

void initialize_fill_prbs(function_wrapper<fill_prbs_aligned_fn>& wrapper)
{
    std::array<uint32_t, fill_buffer_length> buffer;
    wrapper.init(buffer.data(), buffer.size(), 0xffffffff);
}

void initialize_verify_prbs(function_wrapper<verify_prbs_aligned_fn>& wrapper)
{
    std::array<uint32_t, fill_buffer_length> buffer;
    scalar::fill_prbs_aligned(buffer.data(), buffer.size(), 0xffffffff);

    /* Use the first value in the buffer as the seed value for verify */
    wrapper.init(buffer.data(), buffer.size(), ~buffer[0]);
}

functions::functions()
{
    initialize_checksum_ipv4_headers(checksum_ipv4_headers_impl);
    initialize_checksum_ipv4_headers(checksum_ipv4_pseudoheaders_impl);
    initialize_checksum_data(checksum_data_aligned_impl);
    initialize_decode_signatures(decode_signatures_impl);
    initialize_encode_signatures(encode_signatures_impl);
    initialize_fill_step(fill_step_aligned_impl);
    initialize_fill_prbs(fill_prbs_aligned_impl);
    initialize_verify_prbs(verify_prbs_aligned_impl);
}

} // namespace pga
