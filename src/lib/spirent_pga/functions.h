#ifndef _LIB_SPIRENT_PGA_FUNCTIONS_H_
#define _LIB_SPIRENT_PGA_FUNCTIONS_H_

#include <stdint.h>

#include "function_wrapper.h"

using checksum_ipv4_headers_fn = void (*)(const uint8_t*[],
                                          uint16_t,
                                          uint32_t[]);

/*
 * Since the IPv4 header and pseudoheader checksum functions have the same
 * signature, we need some tag structs to differentiate between them.
 */
struct ipv4_header
{};
struct ipv4_pseudoheader
{};

ISPC_FUNCTION_WRAPPER_TAGGED_INIT(ipv4_header,
                                  void,
                                  checksum_ipv4_headers,
                                  const uint8_t*[],
                                  uint16_t,
                                  uint32_t[]);

ISPC_FUNCTION_WRAPPER_TAGGED_INIT(ipv4_pseudoheader,
                                  void,
                                  checksum_ipv4_pseudoheaders,
                                  const uint8_t*[],
                                  uint16_t,
                                  uint32_t[]);

using checksum_data_aligned_fn = uint32_t (*)(const uint32_t[], uint16_t);
ISPC_FUNCTION_WRAPPER_INIT(uint32_t,
                           checksum_data_aligned,
                           const uint32_t[],
                           uint16_t);

using encode_signatures_fn = void (*)(
    uint8_t*[], const uint32_t[], const uint32_t[], uint64_t, int, uint16_t);
ISPC_FUNCTION_WRAPPER_INIT(void,
                           encode_signatures,
                           uint8_t*[],
                           const uint32_t[],
                           const uint32_t[],
                           uint64_t,
                           int,
                           uint16_t);

using decode_signatures_fn = uint16_t (*)(const uint8_t* const[],
                                          uint16_t,
                                          uint32_t[],
                                          uint32_t[],
                                          uint32_t[],
                                          uint32_t[],
                                          int[]);
ISPC_FUNCTION_WRAPPER_INIT(uint16_t,
                           decode_signatures,
                           const uint8_t* const[],
                           uint16_t,
                           uint32_t[],
                           uint32_t[],
                           uint32_t[],
                           uint32_t[],
                           int[]);

using fill_step_aligned_fn = uint8_t (*)(uint32_t[], uint16_t, uint8_t, int8_t);
ISPC_FUNCTION_WRAPPER_INIT(
    uint8_t, fill_step_aligned, uint32_t[], uint16_t, uint8_t, int8_t);

using fill_prbs_aligned_fn = uint32_t (*)(uint32_t[], uint16_t, uint32_t);
ISPC_FUNCTION_WRAPPER_INIT(
    uint32_t, fill_prbs_aligned, uint32_t[], uint16_t, uint32_t);

using unpack_and_sum_indexicals_fn = void (*)(
    const uint32_t[], uint16_t, const uint32_t[], uint16_t, uint64_t*[]);
ISPC_FUNCTION_WRAPPER_INIT(void,
                           unpack_and_sum_indexicals,
                           const uint32_t[],
                           uint16_t,
                           const uint32_t[],
                           uint16_t,
                           uint64_t*[]);

/*
 * Unfortunately, ispc doesn't properly return structs, so this 64 bit value is
 * composed of both the next expected payload value and the bit error count in
 * the format (expected << 32 | bit errors). Note: using non-idempotent function
 * arguments, e.g an output pointer, interferes with proper initialization
 * benchmarking.
 */
using verify_prbs_aligned_fn = uint64_t (*)(const uint32_t[],
                                            uint16_t,
                                            uint32_t);
ISPC_FUNCTION_WRAPPER_INIT(
    uint64_t, verify_prbs_aligned, const uint32_t[], uint16_t, uint32_t);

namespace pga {

template <typename T> class singleton
{
public:
    static T& instance()
    {
        static T instance;
        return instance;
    }

    singleton(const singleton&) = delete;
    singleton& operator=(const singleton) = delete;

protected:
    singleton(){};
};

struct functions : singleton<functions>
{
    functions();

    function_wrapper<checksum_ipv4_headers_fn, ipv4_header>
        checksum_ipv4_headers_impl = {"IPv4 header checksumming", nullptr};
    function_wrapper<checksum_ipv4_headers_fn, ipv4_pseudoheader>
        checksum_ipv4_pseudoheaders_impl = {"IPv4 pseudoheader checksumming",
                                            nullptr};
    function_wrapper<checksum_data_aligned_fn> checksum_data_aligned_impl = {
        "data checksumming", nullptr};

    function_wrapper<encode_signatures_fn> encode_signatures_impl = {
        "signature encodes", nullptr};
    function_wrapper<decode_signatures_fn> decode_signatures_impl = {
        "signature decodes", nullptr};

    function_wrapper<fill_step_aligned_fn> fill_step_aligned_impl = {
        "payload fills", nullptr};
    function_wrapper<fill_prbs_aligned_fn> fill_prbs_aligned_impl = {
        "PRBS generation", nullptr};

    function_wrapper<verify_prbs_aligned_fn> verify_prbs_aligned_impl = {
        "PRBS verification", nullptr};

    function_wrapper<unpack_and_sum_indexicals_fn>
        unpack_and_sum_indexicals_impl = {"unpack and sum indexicals", nullptr};
};

} // namespace pga

#endif /* _LIB_SPIRENT_PGA_FUNCTIONS_H_ */
