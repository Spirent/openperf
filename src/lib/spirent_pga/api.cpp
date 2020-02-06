#include "api.h"
#include "common/checksum.h"
#include "common/fill.h"
#include "common/prbs.h"
#include "common/signature.h"
#include "common/verify.h"
#include "functions.h"
#include "instruction_set.h"

static void pga_log_implementation_info(FILE* output,
                                        std::string_view function,
                                        std::string_view instructions)
{
    fprintf(output,
            "spirent_pga: using %.*s instructions for %.*s\n",
            static_cast<int>(instructions.size()),
            instructions.data(),
            static_cast<int>(function.size()),
            function.data());
}

extern "C" {

void pga_init()
{
    [[maybe_unused]] auto& functions = pga::functions::instance();
}

void pga_log_info(FILE* output)
{
    auto& functions = pga::functions::instance();

    pga_log_implementation_info(
        output,
        functions.checksum_ipv4_headers_impl.name,
        pga::instruction_set::to_string(
            pga::get_instruction_set(functions.checksum_ipv4_headers_impl)));
    pga_log_implementation_info(
        output,
        functions.checksum_ipv4_pseudoheaders_impl.name,
        pga::instruction_set::to_string(pga::get_instruction_set(
            functions.checksum_ipv4_pseudoheaders_impl)));
    pga_log_implementation_info(
        output,
        functions.checksum_data_aligned_impl.name,
        pga::instruction_set::to_string(
            pga::get_instruction_set(functions.checksum_data_aligned_impl)));
    pga_log_implementation_info(
        output,
        functions.decode_signatures_impl.name,
        pga::instruction_set::to_string(
            pga::get_instruction_set(functions.decode_signatures_impl)));
    pga_log_implementation_info(
        output,
        functions.encode_signatures_impl.name,
        pga::instruction_set::to_string(
            pga::get_instruction_set(functions.encode_signatures_impl)));
    pga_log_implementation_info(
        output,
        functions.fill_step_aligned_impl.name,
        pga::instruction_set::to_string(
            pga::get_instruction_set(functions.fill_step_aligned_impl)));
    pga_log_implementation_info(
        output,
        functions.fill_prbs_aligned_impl.name,
        pga::instruction_set::to_string(
            pga::get_instruction_set(functions.fill_prbs_aligned_impl)));
    pga_log_implementation_info(
        output,
        functions.verify_prbs_aligned_impl.name,
        pga::instruction_set::to_string(
            pga::get_instruction_set(functions.verify_prbs_aligned_impl)));
    pga_log_implementation_info(
        output,
        functions.unpack_and_sum_indexicals_impl.name,
        pga::instruction_set::to_string(pga::get_instruction_set(
            functions.unpack_and_sum_indexicals_impl)));
}

int pga_signature_flag(enum pga_signature_prbs prbs,
                       enum pga_signature_timestamp ts)
{
    int flags = 0;
    if (prbs == pga_signature_prbs::enable) { flags += (1 << 1); }
    if (ts == pga_signature_timestamp::last) { flags += (1 << 0); }

    return (flags);
}

enum pga_signature_prbs pga_prbs_flag(int flags)
{
    return (flags & (1 << 1) ? pga_signature_prbs::enable
                             : pga_signature_prbs::disable);
}

enum pga_signature_timestamp pga_timestamp_flag(int flags)
{
    return (flags & (1 << 0) ? pga_signature_timestamp::last
                             : pga_signature_timestamp::first);
}

enum pga_signature_status pga_status_flag(int flags)
{
    return (flags & (1 << 2) ? pga_signature_status::valid
                             : pga_signature_status::invalid);
}

uint16_t pga_signatures_crc_filter(const uint8_t* payloads[],
                                   uint16_t count,
                                   int crc_matches[])
{
    return (pga::signature::crc_filter(payloads, count, crc_matches));
}

uint16_t pga_signatures_decode(const uint8_t* payloads[],
                               uint16_t count,
                               uint32_t stream_ids[],
                               uint32_t sequence_numbers[],
                               uint64_t tx_timestamps[],
                               int flags[])
{
    return (pga::signature::decode(
        payloads, count, stream_ids, sequence_numbers, tx_timestamps, flags));
}

void pga_signatures_encode(uint8_t* destinations[],
                           const uint32_t stream_ids[],
                           const uint32_t sequence_numbers[],
                           uint64_t timestamp,
                           int flags,
                           uint16_t count)
{
    pga::signature::encode(
        destinations, stream_ids, sequence_numbers, timestamp, flags, count);
}

void pga_fill_const(uint8_t* payloads[],
                    uint16_t lengths[],
                    uint16_t count,
                    uint8_t base)
{
    for (uint16_t i = 0; i < count; i++) {
        pga::fill::fixed(payloads[i], lengths[i], base);
    }
}

void pga_fill_decr(uint8_t* payloads[],
                   uint16_t lengths[],
                   uint16_t count,
                   uint8_t base)
{
    for (uint16_t i = 0; i < count; i++) {
        pga::fill::decr(payloads[i], lengths[i], base);
    }
}

void pga_fill_incr(uint8_t* payloads[],
                   uint16_t lengths[],
                   uint16_t count,
                   uint8_t base)
{
    for (uint16_t i = 0; i < count; i++) {
        pga::fill::incr(payloads[i], lengths[i], base);
    }
}

uint32_t pga_fill_prbs(uint8_t* payloads[],
                       uint16_t lengths[],
                       uint16_t count,
                       uint32_t seed)
{
    for (uint16_t i = 0; i < count; i++) {
        seed = pga::fill::prbs(payloads[i], lengths[i], seed);
    }

    return (seed);
}

bool pga_verify_prbs(uint8_t* payloads[],
                     uint16_t lengths[],
                     uint16_t count,
                     uint32_t bit_errors[])
{
    for (uint16_t i = 0; i < count; i++) {
        bit_errors[i] = pga::verify::prbs(payloads[i], lengths[i]);
    }

    return (std::any_of(
        bit_errors, bit_errors + count, [](auto& value) { return (value); }));
}

void pga_checksum_ipv4_headers(const uint8_t* ipv4_headers[],
                               uint16_t count,
                               uint32_t checksums[])
{
    pga::checksum::ipv4_headers(ipv4_headers, count, checksums);
}

void pga_checksum_ipv4_tcpudp(const uint8_t* ipv4_headers[],
                              uint16_t count,
                              uint32_t checksums[])
{
    pga::checksum::ipv4_tcpudp(ipv4_headers, count, checksums);
}

void pga_unpack_and_sum_indexicals(const uint32_t indexicals[],
                                   uint16_t nb_indexicals,
                                   const uint32_t masks[],
                                   uint16_t nb_masks,
                                   uint64_t* counters[])
{
    auto& functions = pga::functions::instance();
    functions.unpack_and_sum_indexicals_impl(
        indexicals, nb_indexicals, masks, nb_masks, counters);
}
}
