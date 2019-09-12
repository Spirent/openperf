#include <algorithm>
#include <array>
#include "functions.h"

namespace pga {

constexpr uint16_t nb_signatures = 128;
constexpr uint16_t signature_length = 16;
constexpr uint16_t fill_buffer_length = 512;

/***
 * Implementation function initialization routines.
 * Note: we intentionally use std::array<> so that library initialization doesn't
 * require any memory allocations.
 ***/

void initialize_encode_signatures(function_wrapper<encode_signatures_fn>& wrapper)
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
                 0xdeadbeef, 0x0, nb_signatures);
}

void initialize_decode_signatures(function_wrapper<decode_signatures_fn>& wrapper)
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
                              0xdeadbeef, 0x0, nb_signatures);

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
    initialize_decode_signatures(decode_signatures_impl);
    initialize_encode_signatures(encode_signatures_impl);
    initialize_fill_step(fill_step_aligned_impl);
    initialize_fill_prbs(fill_prbs_aligned_impl);
    initialize_verify_prbs(verify_prbs_aligned_impl);
}

}
