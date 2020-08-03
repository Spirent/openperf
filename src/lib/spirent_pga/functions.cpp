#include <algorithm>
#include <array>
#include "functions.h"

namespace pga {

constexpr uint16_t nb_items = 32;
constexpr uint16_t nb_ipv4_headers = nb_items;
constexpr uint16_t ipv4_header_length = 20; /* octets */

constexpr uint16_t nb_signatures = nb_items;
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
    std::array<uint32_t, nb_signatures> timestamps_hi;
    std::array<uint32_t, nb_signatures> timestamps_lo;
    std::array<int, nb_signatures> flags{0};

    for (auto i = 0; i < nb_signatures; i++) {
        signature_ptrs[i] = &signatures[i][0];
        stream_ids[i] = 0xc0ffee00 + i;
        sequence_nums[i] = i;
        timestamps_hi[i] = 0xdead;
        timestamps_lo[i] = 0xbeef000 + i;
    }

    wrapper.init(signature_ptrs.data(),
                 stream_ids.data(),
                 sequence_nums.data(),
                 timestamps_hi.data(),
                 timestamps_lo.data(),
                 flags.data(),
                 nb_signatures);
}

void initialize_decode_signatures(
    function_wrapper<decode_signatures_fn>& wrapper)
{
    std::array<uint8_t[signature_length], nb_signatures> signatures;
    std::array<uint8_t*, nb_signatures> signature_ptrs;
    std::array<uint32_t, nb_signatures> stream_ids;
    std::array<uint32_t, nb_signatures> sequence_nums;
    std::array<uint32_t, nb_signatures> timestamps_hi;
    std::array<uint32_t, nb_signatures> timestamps_lo;
    std::array<int, nb_signatures> flags{0};

    for (auto i = 0; i < nb_signatures; i++) {
        signature_ptrs[i] = &signatures[i][0];
        stream_ids[i] = 0xc0ffee00 + i;
        sequence_nums[i] = i;
        timestamps_hi[i] = 0xdead;
        timestamps_lo[i] = 0xbeef000 + i;
    }

    /*
     * We need some valid signatures to decode and the scalar functions
     * are always available.
     */
    scalar::encode_signatures(signature_ptrs.data(),
                              stream_ids.data(),
                              sequence_nums.data(),
                              timestamps_hi.data(),
                              timestamps_lo.data(),
                              flags.data(),
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

void initialize_fill_constant(
    function_wrapper<fill_constant_aligned_fn>& wrapper)
{
    std::array<uint32_t, fill_buffer_length> buffer;
    wrapper.init(buffer.data(), buffer.size(), 0xdeadbeef);
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

void initialize_unpack_and_sum_indexicals(
    function_wrapper<unpack_and_sum_indexicals_fn>& wrapper)
{
    std::array<uint32_t, nb_items> fields;
    std::fill_n(fields.data(), fields.size(), 0x33333333);

    std::array<uint32_t, 3> masks = {0xf, 0xf0, 0x300};

    std::array<uint64_t, 16> counter_a;
    std::array<uint64_t, 16> counter_b;
    std::array<uint64_t, 16> counter_c;
    std::fill_n(counter_a.data(), counter_a.size(), 0);
    std::fill_n(counter_b.data(), counter_b.size(), 0);
    std::fill_n(counter_c.data(), counter_c.size(), 0);

    std::array<uint64_t*, 3> counters = {
        counter_a.data(), counter_b.data(), counter_c.data()};

    wrapper.init(fields.data(),
                 fields.size(),
                 masks.data(),
                 masks.size(),
                 counters.data());
}

functions::functions()
{
    initialize_checksum_ipv4_headers(checksum_ipv4_headers_impl);
    initialize_checksum_ipv4_headers(checksum_ipv4_pseudoheaders_impl);
    initialize_checksum_data(checksum_data_aligned_impl);
    initialize_decode_signatures(decode_signatures_impl);
    initialize_encode_signatures(encode_signatures_impl);
    initialize_fill_constant(fill_constant_aligned_impl);
    initialize_fill_step(fill_step_aligned_impl);
    initialize_fill_prbs(fill_prbs_aligned_impl);
    initialize_verify_prbs(verify_prbs_aligned_impl);
    initialize_unpack_and_sum_indexicals(unpack_and_sum_indexicals_impl);
}

} // namespace pga
