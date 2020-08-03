#include "spirent_pga/functions.h"
#include "fill.h"
#include "prbs.h"

#include "arpa/inet.h"

namespace pga::fill {

uint32_t prbs(uint8_t payload[], uint16_t length, uint32_t seed)
{
    auto& functions = functions::instance();
    uint16_t idx = 0;

    /*
     * If the payload isn't aligned to a 32-bit boundary, handle partial
     * updates until we get to a 32-bit boundary.
     */
    switch (reinterpret_cast<uintptr_t>(payload) & 0x3) {
    case 0:
        break; /* no-op */
    case 1: {
        auto next = prbs::step(seed);
        payload[idx++] = (~seed >> 8) & 0xff;
        payload[idx++] = (~seed >> 16) & 0xff;
        payload[idx++] = (~seed >> 24) & 0xff;
        seed = seed << 24 | next >> 8;
        break;
    }
    case 2: {
        auto next = prbs::step(seed);
        payload[idx++] = (~seed >> 16) & 0xff;
        payload[idx++] = (~seed >> 24) & 0xff;
        seed = seed << 16 | next >> 16;
        break;
    }
    case 3: {
        auto next = prbs::step(seed);
        payload[idx++] = (~seed >> 24) & 0xff;
        seed = seed << 8 | next >> 24;
        break;
    }
    }

    auto aligned_length = (length - idx) / 4;
    seed = functions.fill_prbs_aligned_impl(
        reinterpret_cast<uint32_t*>(&payload[idx]), aligned_length, seed);

    /*
     * Check and handle any remaining unaligned payload past the last
     * 32-bit boundary
     */
    idx += aligned_length * 4;
    switch ((length - idx) & 0x3) {
    case 0:
        break; /* no-op */
    case 1: {
        auto next = prbs::step(seed);
        payload[idx++] = (~seed >> 24) & 0xff;
        seed = seed << 24 | next >> 8;
        break;
    }
    case 2: {
        auto next = prbs::step(seed);
        payload[idx++] = (~seed >> 24) & 0xff;
        payload[idx++] = (~seed >> 16) & 0xff;
        seed = seed << 16 | next >> 16;
        break;
    }
    case 3: {
        auto next = prbs::step(seed);
        payload[idx++] = (~seed >> 24) & 0xff;
        payload[idx++] = (~seed >> 16) & 0xff;
        payload[idx++] = (~seed >> 8) & 0xff;
        seed = seed << 8 | next >> 24;
        break;
    }
    }

    return (seed);
}

void fixed(uint8_t payload[], uint16_t length, uint16_t value)
{
    auto& functions = functions::instance();
    uint16_t idx = 0;

    /*
     * If the payload isn't aligned to a 32-bit boundary, handle partial
     * updates until we get to a 32-bit boundary.
     */
    switch (reinterpret_cast<uintptr_t>(payload) & 0x3) {
    case 0:
        break; /* no-op */
    case 1: {
        payload[idx++] = value >> 8;
        payload[idx++] = value & 0xff;
        payload[idx++] = value >> 8;
        value = (value << 8) | (value >> 8);
        break;
    }
    case 2: {
        payload[idx++] = value >> 8;
        payload[idx++] = value & 0xff;
        break;
    }
    case 3: {
        payload[idx++] = value >> 8;
        value = (value << 8) | (value >> 8);
        break;
    }
    }

    auto aligned_length = (length - idx) / 4;
    functions.fill_constant_aligned_impl(
        reinterpret_cast<uint32_t*>(&payload[idx]),
        aligned_length,
        htonl((value << 16) | value));

    /*
     * Check and handle any remaining unaligned payload past the last
     * 32-bit boundary
     */
    idx += aligned_length * 4;
    switch ((length - idx) & 0x3) {
    case 0:
        break; /* no-op */
    case 1: {
        payload[idx++] = value >> 8;
        break;
    }
    case 2: {
        payload[idx++] = value >> 8;
        payload[idx++] = value & 0xff;
        break;
    }
    case 3: {
        payload[idx++] = value >> 8;
        payload[idx++] = value & 0xff;
        payload[idx++] = value >> 8;
        break;
    }
    }
}

void decr(uint8_t payload[], uint16_t length, uint8_t value)
{
    uint16_t idx = 0;
    auto& functions = functions::instance();

    while (idx < length && reinterpret_cast<uintptr_t>(&payload[idx]) & 0x3) {
        payload[idx++] = value--;
    }

    auto aligned_length = (length - idx) / 4;
    value = functions.fill_step_aligned_impl(
        reinterpret_cast<uint32_t*>(&payload[idx]), aligned_length, value, -1);

    idx += aligned_length * 4;
    while (idx < length && reinterpret_cast<uintptr_t>(&payload[idx]) & 0x3) {
        payload[idx++] = value--;
    }
}

void incr(uint8_t payload[], uint16_t length, uint8_t value)
{
    uint16_t idx = 0;
    auto& functions = functions::instance();

    while (idx < length && reinterpret_cast<uintptr_t>(&payload[idx]) & 0x3) {
        payload[idx++] = value++;
    }

    auto aligned_length = (length - idx) / 4;
    value = functions.fill_step_aligned_impl(
        reinterpret_cast<uint32_t*>(&payload[idx]), aligned_length, value, 1);

    idx += aligned_length * 4;
    while (idx < length && reinterpret_cast<uintptr_t>(&payload[idx]) & 0x3) {
        payload[idx++] = value--;
    }
}

} // namespace pga::fill
