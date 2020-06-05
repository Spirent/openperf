#include <algorithm>
#include <cassert>
#include <numeric>
#include <set>
#include <vector>
#include <memory>

#include "core/op_log.h"
#include "packetio/bpf/bpf.hpp"
#include "packetio/packet_buffer.hpp"

namespace openperf::packetio::bpf {

constexpr int BFP_PKTFLAG_SIGNATURE = 0x1;

constexpr int BPF_MEM_PKTFLAGS = 0;
constexpr int BPF_MEM_STREAM_ID = 1;

inline void bpf_arg_init(bpf_ctx_t& ctx,
                         bpf_args_t& args,
                         const packet::packet_buffer* packet)
{
    ctx.preinited = BPF_MEMWORD_INIT(BPF_MEM_PKTFLAGS)
                    | BPF_MEMWORD_INIT(BPF_MEM_STREAM_ID);

    args.buflen = args.wirelen = packet::length(packet);
    args.pkt = reinterpret_cast<const uint8_t*>(packet::to_data(packet));
    auto stream_id = packet::signature_stream_id(packet);
    args.mem[BPF_MEM_PKTFLAGS] = 0;
    if (stream_id) {
        args.mem[BPF_MEM_PKTFLAGS] |= BFP_PKTFLAG_SIGNATURE;
        args.mem[BPF_MEM_STREAM_ID] = stream_id.value();
    }
}

uint16_t bpf_all_filter_func(bpf& bpf,
                             const packet::packet_buffer* const packets[],
                             uint64_t results[],
                             uint16_t length)
{
    std::fill(results, results + length, 1);
    return length;
}

uint16_t bpf_all_find_next_func(bpf& bpf,
                                const packet::packet_buffer* const packets[],
                                uint16_t length,
                                uint16_t offset)
{
    return offset;
}

uint16_t bpf_vm_filter_func(bpf& bpf,
                            const packet::packet_buffer* const packets[],
                            uint64_t results[],
                            uint16_t length)
{
    uint32_t mem[BPF_MEMWORDS];
    bpf_ctx_t ctx{.copfuncs = nullptr,
                  .nfuncs = 0,
                  .extwords = BPF_MEMWORDS,
                  .preinited = 0};
    bpf_args_t args{
        .pkt = nullptr, .wirelen = 0, .buflen = 0, .mem = mem, .arg = nullptr};
    auto insns = &bpf.get_prog()[0];

    std::transform(packets, packets + length, results, [&](auto packet) {
        bpf_arg_init(ctx, args, packet);
        return op_bpf_filter_ext(&ctx, insns, &args);
    });

    return length;
}

uint16_t bpf_vm_find_next_func(bpf& bpf,
                               const packet::packet_buffer* const packets[],
                               uint16_t length,
                               uint16_t offset)
{
    uint32_t mem[BPF_MEMWORDS];
    bpf_ctx_t ctx{.copfuncs = nullptr,
                  .nfuncs = 0,
                  .extwords = BPF_MEMWORDS,
                  .preinited = 0};
    bpf_args_t args{
        .pkt = nullptr, .wirelen = 0, .buflen = 0, .mem = mem, .arg = nullptr};
    auto insns = &bpf.get_prog()[0];

    auto found =
        std::find_if(packets + offset, packets + length, [&](auto packet) {
            bpf_arg_init(ctx, args, packet);
            return (op_bpf_filter_ext(&ctx, insns, &args));
        });

    return std::distance(packets, found);
}

uint16_t bpf_jit_filter_func(bpf& bpf,
                             const packet::packet_buffer* const packets[],
                             uint64_t results[],
                             uint16_t length)
{
    uint32_t mem[BPF_MEMWORDS];
    bpf_ctx_t ctx{.copfuncs = nullptr,
                  .nfuncs = 0,
                  .extwords = BPF_MEMWORDS,
                  .preinited = 0};
    bpf_args_t args{
        .pkt = nullptr, .wirelen = 0, .buflen = 0, .mem = mem, .arg = nullptr};
    auto filter_func = bpf.get_filter_func();

    std::transform(packets, packets + length, results, [&](auto packet) {
        bpf_arg_init(ctx, args, packet);
        return filter_func(&ctx, &args);
    });

    return length;
}

uint16_t bpf_jit_find_next_func(bpf& bpf,
                                const packet::packet_buffer* const packets[],
                                uint16_t length,
                                uint16_t offset)
{
    uint32_t mem[BPF_MEMWORDS];
    bpf_ctx_t ctx{.copfuncs = nullptr,
                  .nfuncs = 0,
                  .extwords = BPF_MEMWORDS,
                  .preinited = 0};
    bpf_args_t args{
        .pkt = nullptr, .wirelen = 0, .buflen = 0, .mem = mem, .arg = nullptr};
    auto filter_func = bpf.get_filter_func();

    auto found =
        std::find_if(packets + offset, packets + length, [&](auto packet) {
            bpf_arg_init(ctx, args, packet);
            return filter_func(&ctx, &args);
        });

    return std::distance(packets, found);
}

bpf_program_ptr bpf_compile(std::string_view filter, int link_type)
{
    std::string filter_str(filter);

    auto prog = bpf_program_ptr(new bpf_program{}, [](bpf_program* prog) {
        if (prog->bf_insns) { pcap_freecode(prog); }
        delete prog;
    });

    if (pcap_compile_nopcap(0xffff,
                            link_type,
                            prog.get(),
                            const_cast<char*>(filter_str.c_str()),
                            1,
                            UINT32_MAX)
        < 0) {
        prog.reset();
    }

    return prog;
}

bpf::bpf()
    : m_exec_burst_func(bpf_all_filter_func)
    , m_find_next_func(bpf_all_find_next_func)
{}

bpf::bpf(std::string_view filter_str, int link_type)
    : m_exec_burst_func(bpf_all_filter_func)
    , m_find_next_func(bpf_all_find_next_func)
{
    if (!parse(filter_str, link_type)) {
        throw std::invalid_argument("BPF filter_str is not valid");
    }
}

bpf::bpf(const bpf_insn* insns, unsigned int len)
    : m_exec_burst_func(bpf_all_filter_func)
    , m_find_next_func(bpf_all_find_next_func)
{
    if (!set_prog(insns, len)) {
        throw std::invalid_argument("BPF insns not valid");
    }
}

bool bpf::set_prog(const bpf_insn* insns, unsigned int len)
{
    bpf_ctx_t ctx{.copfuncs = nullptr,
                  .nfuncs = 0,
                  .extwords = BPF_MEMWORDS,
                  .preinited = 0};

    ctx.preinited = BPF_MEMWORD_INIT(BPF_MEM_PKTFLAGS)
                    | BPF_MEMWORD_INIT(BPF_MEM_STREAM_ID);

    if (!op_bpf_validate_ext(&ctx, insns, len)) {
        OP_LOG(OP_LOG_ERROR, "Unable to validate BPF program");
        return false;
    }

    m_insn.resize(0);
    m_insn.reserve(len);
    std::copy(insns, insns + len, std::back_inserter(m_insn));

    m_jit = bpf_jit(insns, len);

    if (!m_jit) {
        OP_LOG(OP_LOG_DEBUG, "Unable to generate BPF JIT code");
        m_exec_burst_func = bpf_vm_filter_func;
        m_find_next_func = bpf_vm_find_next_func;
    } else {
        m_exec_burst_func = bpf_jit_filter_func;
        m_find_next_func = bpf_jit_find_next_func;
    }
    return true;
}

bool bpf::parse(std::string_view filter_str, int link_type)
{
    auto prog = bpf_compile(filter_str, link_type);
    if (!prog) return false;

    return set_prog(prog->bf_insns, prog->bf_len);
}

} // namespace openperf::packetio::bpf