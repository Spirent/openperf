#ifndef _LIB_SPIRENT_PGA_FUNCTIONS_H_
#define _LIB_SPIRENT_PGA_FUNCTIONS_H_

#include <stdint.h>

#include "function_wrapper.h"

using encode_signatures_fn = void (*)(uint8_t*[], const uint32_t[], const uint32_t[],
                                     uint64_t, int, uint16_t);
ISPC_FUNCTION_WRAPPER_INIT(void, encode_signatures, uint8_t*[], const uint32_t[], const uint32_t[],
                           uint64_t, int, uint16_t);

using decode_signatures_fn = uint16_t (*)(const uint8_t* const[], uint16_t,
                                          uint32_t[], uint32_t[], uint32_t[], uint32_t[], int[]);
ISPC_FUNCTION_WRAPPER_INIT(uint16_t, decode_signatures, const uint8_t* const[], uint16_t,
                           uint32_t[], uint32_t[], uint32_t[], uint32_t[], int[]);

using fill_step_aligned_fn = uint8_t (*)(uint32_t[], uint16_t, uint8_t, int8_t);
ISPC_FUNCTION_WRAPPER_INIT(uint8_t, fill_step_aligned, uint32_t[], uint16_t, uint8_t, int8_t);

using fill_prbs_aligned_fn = uint32_t (*)(uint32_t[], uint16_t, uint32_t);
ISPC_FUNCTION_WRAPPER_INIT(uint32_t, fill_prbs_aligned, uint32_t[], uint16_t, uint32_t);

/*
 * Unfortunately, ispc doesn't properly return structs, so this 64 bit value is
 * composed of both the next expected payload value and the bit error count in the format
 * (expected << 32 | bit errors).
 * Note: using non-idempotent function arguments, e.g an output pointer, interferes with
 * proper initialization benchmarking.
 */
using verify_prbs_aligned_fn = uint64_t (*)(const uint32_t[], uint16_t, uint32_t);
ISPC_FUNCTION_WRAPPER_INIT(uint64_t, verify_prbs_aligned, const uint32_t[], uint16_t, uint32_t);

namespace pga {

template <typename T>
class singleton {
public:
    static T& instance()
    {
        static T instance;
        return instance;
    }

    singleton(const singleton&) = delete;
    singleton& operator=(const singleton) = delete;

protected:
    singleton() {};
};

struct functions : singleton<functions>
{
    functions();

    function_wrapper<encode_signatures_fn> encode_signatures_impl = {"signature encodes", nullptr };
    function_wrapper<decode_signatures_fn> decode_signatures_impl = {"signature decodes", nullptr };

    function_wrapper<fill_step_aligned_fn> fill_step_aligned_impl = {"payload fills", nullptr };
    function_wrapper<fill_prbs_aligned_fn> fill_prbs_aligned_impl = {"PRBS generation", nullptr };

    function_wrapper<verify_prbs_aligned_fn> verify_prbs_aligned_impl = {"PRBS verification", nullptr };
};

}

#endif /* _LIB_SPIRENT_PGA_FUNCTIONS_H_ */
