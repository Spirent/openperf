#include <algorithm>
#include <numeric>
#include <vector>
#include <memory>
#include <sstream>

#include "core/op_log.h"
#include "packet/bpf/bpf.hpp"
#include "packet/bpf/bpf_parse.hpp"
#include "packet/bpf/bpf_build.hpp"
#include "packetio/packet_buffer.hpp"

namespace openperf::packet::bpf {

constexpr bpf_memword_init_t bpf_ctx_preinited()
{
    return BPF_MEMWORD_INIT(BPF_MEM_PKTFLAGS)
           | BPF_MEMWORD_INIT(BPF_MEM_STREAM_ID);
}

inline void bpf_arg_init(bpf_args_t& args,
                         const packetio::packet::packet_buffer* packet)
{
    args.buflen = args.wirelen = packetio::packet::length(packet);
    args.pkt =
        reinterpret_cast<const uint8_t*>(packetio::packet::to_data(packet));
    auto stream_id = packetio::packet::signature_stream_id(packet);
    args.mem[BPF_MEM_PKTFLAGS] = 0;
    if (stream_id.has_value()) {
        args.mem[BPF_MEM_PKTFLAGS] |= BPF_PKTFLAG_SIGNATURE;
        args.mem[BPF_MEM_STREAM_ID] = stream_id.value();
        if (packetio::packet::prbs_bit_errors(packet))
            args.mem[BPF_MEM_PKTFLAGS] |= BPF_PKTFLAG_PRBS_ERROR;
    }

    // TODO: Add packet_buffer support for FCS error
    // args.mem[BPF_MEM_PKTFLAGS] |= BPF_PKTFLAG_FCS_ERROR;
    // TODO: Add packet_buffer support for ICMP checksum error
    // args.mem[BPF_MEM_PKTFLAGS] |= BPF_PKTFLAG_ICMP_CHKSUM_ERROR;
    if (packetio::packet::ipv4_checksum_error(packet))
        args.mem[BPF_MEM_PKTFLAGS] |= BPF_PKTFLAG_IP_CHKSUM_ERROR;
    if (packetio::packet::tcp_checksum_error(packet))
        args.mem[BPF_MEM_PKTFLAGS] |= BPF_PKTFLAG_TCP_CHKSUM_ERROR;
    if (packetio::packet::udp_checksum_error(packet))
        args.mem[BPF_MEM_PKTFLAGS] |= BPF_PKTFLAG_UDP_CHKSUM_ERROR;
}

uint16_t
bpf_all_filter_func(bpf&,
                    const packetio::packet::packet_buffer* const packets[],
                    const packetio::packet::packet_buffer* results[],
                    uint16_t length)
{
    std::copy(packets, packets + length, results);
    return length;
}

uint16_t bpf_all_exec_func(bpf&,
                           const packetio::packet::packet_buffer* const[],
                           uint64_t results[],
                           uint16_t length)
{
    std::fill(results, results + length, 1);
    return length;
}

uint16_t bpf_all_find_next_func(bpf&,
                                const packetio::packet::packet_buffer* const[],
                                uint16_t,
                                uint16_t offset)
{
    return offset;
}

bpf_funcs bpf_all_funcs{
    bpf_all_filter_func, bpf_all_exec_func, bpf_all_find_next_func};

uint16_t
bpf_vm_filter_func(bpf& bpf,
                   const packetio::packet::packet_buffer* const packets[],
                   const packetio::packet::packet_buffer* results[],
                   uint16_t length)
{
    uint32_t mem[BPF_MEMWORDS];
    bpf_ctx_t ctx{.copfuncs = nullptr,
                  .nfuncs = 0,
                  .extwords = BPF_MEMWORDS,
                  .preinited = bpf_ctx_preinited()};
    bpf_args_t args{
        .pkt = nullptr, .wirelen = 0, .buflen = 0, .mem = mem, .arg = nullptr};
    auto insns = &bpf.get_prog()[0];

    auto results_end =
        std::copy_if(packets, packets + length, results, [&](auto packet) {
            bpf_arg_init(args, packet);
            return op_bpf_filter_ext(&ctx, insns, &args);
        });

    return std::distance(results, results_end);
}

uint16_t
bpf_vm_exec_func(bpf& bpf,
                 const packetio::packet::packet_buffer* const packets[],
                 uint64_t results[],
                 uint16_t length)
{
    uint32_t mem[BPF_MEMWORDS];
    bpf_ctx_t ctx{.copfuncs = nullptr,
                  .nfuncs = 0,
                  .extwords = BPF_MEMWORDS,
                  .preinited = bpf_ctx_preinited()};
    bpf_args_t args{
        .pkt = nullptr, .wirelen = 0, .buflen = 0, .mem = mem, .arg = nullptr};
    auto insns = &bpf.get_prog()[0];

    std::transform(packets, packets + length, results, [&](auto packet) {
        bpf_arg_init(args, packet);
        return op_bpf_filter_ext(&ctx, insns, &args);
    });

    return length;
}

uint16_t
bpf_vm_find_next_func(bpf& bpf,
                      const packetio::packet::packet_buffer* const packets[],
                      uint16_t length,
                      uint16_t offset)
{
    uint32_t mem[BPF_MEMWORDS];
    bpf_ctx_t ctx{.copfuncs = nullptr,
                  .nfuncs = 0,
                  .extwords = BPF_MEMWORDS,
                  .preinited = bpf_ctx_preinited()};
    bpf_args_t args{
        .pkt = nullptr, .wirelen = 0, .buflen = 0, .mem = mem, .arg = nullptr};
    auto insns = &bpf.get_prog()[0];

    auto found =
        std::find_if(packets + offset, packets + length, [&](auto packet) {
            bpf_arg_init(args, packet);
            return (op_bpf_filter_ext(&ctx, insns, &args));
        });

    return std::distance(packets, found);
}

bpf_funcs bpf_vm_funcs{
    bpf_vm_filter_func, bpf_vm_exec_func, bpf_vm_find_next_func};

uint16_t
bpf_jit_filter_func(bpf& bpf,
                    const packetio::packet::packet_buffer* const packets[],
                    const packetio::packet::packet_buffer* results[],
                    uint16_t length)
{
    uint32_t mem[BPF_MEMWORDS];
    bpf_ctx_t ctx{.copfuncs = nullptr,
                  .nfuncs = 0,
                  .extwords = BPF_MEMWORDS,
                  .preinited = bpf_ctx_preinited()};
    bpf_args_t args{
        .pkt = nullptr, .wirelen = 0, .buflen = 0, .mem = mem, .arg = nullptr};
    auto filter_func = bpf.get_filter_func();

    auto results_end =
        std::copy_if(packets, packets + length, results, [&](auto packet) {
            bpf_arg_init(args, packet);
            return filter_func(&ctx, &args);
        });

    return std::distance(results, results_end);
}

uint16_t
bpf_jit_exec_func(bpf& bpf,
                  const packetio::packet::packet_buffer* const packets[],
                  uint64_t results[],
                  uint16_t length)
{
    uint32_t mem[BPF_MEMWORDS];
    bpf_ctx_t ctx{.copfuncs = nullptr,
                  .nfuncs = 0,
                  .extwords = BPF_MEMWORDS,
                  .preinited = bpf_ctx_preinited()};
    bpf_args_t args{
        .pkt = nullptr, .wirelen = 0, .buflen = 0, .mem = mem, .arg = nullptr};
    auto filter_func = bpf.get_filter_func();

    std::transform(packets, packets + length, results, [&](auto packet) {
        bpf_arg_init(args, packet);
        return filter_func(&ctx, &args);
    });

    return length;
}

uint16_t
bpf_jit_find_next_func(bpf& bpf,
                       const packetio::packet::packet_buffer* const packets[],
                       uint16_t length,
                       uint16_t offset)
{
    uint32_t mem[BPF_MEMWORDS];
    bpf_ctx_t ctx{.copfuncs = nullptr,
                  .nfuncs = 0,
                  .extwords = BPF_MEMWORDS,
                  .preinited = bpf_ctx_preinited()};
    bpf_args_t args{
        .pkt = nullptr, .wirelen = 0, .buflen = 0, .mem = mem, .arg = nullptr};
    auto filter_func = bpf.get_filter_func();

    auto found =
        std::find_if(packets + offset, packets + length, [&](auto packet) {
            bpf_arg_init(args, packet);
            return filter_func(&ctx, &args);
        });

    return std::distance(packets, found);
}

bpf_funcs bpf_jit_funcs{
    bpf_jit_filter_func, bpf_jit_exec_func, bpf_jit_find_next_func};

uint16_t
bpf_sig_filter_func(bpf&,
                    const packetio::packet::packet_buffer* const packets[],
                    const packetio::packet::packet_buffer* results[],
                    uint16_t length)
{
    auto results_end =
        std::copy_if(packets, packets + length, results, [&](auto packet) {
            auto stream_id = packetio::packet::signature_stream_id(packet);
            return stream_id.has_value();
        });
    return std::distance(results, results_end);
}

uint16_t
bpf_sig_exec_func(bpf&,
                  const packetio::packet::packet_buffer* const packets[],
                  uint64_t results[],
                  uint16_t length)
{
    std::transform(packets, packets + length, results, [&](auto packet) {
        auto stream_id = packetio::packet::signature_stream_id(packet);
        return stream_id.has_value();
    });
    return length;
}

uint16_t
bpf_sig_find_next_func(bpf&,
                       const packetio::packet::packet_buffer* const packets[],
                       uint16_t length,
                       uint16_t offset)
{
    auto found =
        std::find_if(packets + offset, packets + length, [&](auto packet) {
            auto stream_id = packetio::packet::signature_stream_id(packet);
            return stream_id.has_value();
        });
    return std::distance(packets, found);
}

bpf_funcs bpf_sig_funcs{
    bpf_sig_filter_func, bpf_sig_exec_func, bpf_sig_find_next_func};

uint16_t
bpf_no_sig_filter_func(bpf&,
                       const packetio::packet::packet_buffer* const packets[],
                       const packetio::packet::packet_buffer* results[],
                       uint16_t length)
{
    auto results_end =
        std::copy_if(packets, packets + length, results, [&](auto packet) {
            auto stream_id = packetio::packet::signature_stream_id(packet);
            return !stream_id.has_value();
        });
    return std::distance(results, results_end);
}

uint16_t
bpf_no_sig_exec_func(bpf&,
                     const packetio::packet::packet_buffer* const packets[],
                     uint64_t results[],
                     uint16_t length)
{
    std::transform(packets, packets + length, results, [&](auto packet) {
        auto stream_id = packetio::packet::signature_stream_id(packet);
        return !stream_id.has_value();
    });
    return length;
}

uint16_t bpf_no_sig_find_next_func(
    bpf&,
    const packetio::packet::packet_buffer* const packets[],
    uint16_t length,
    uint16_t offset)
{
    auto found =
        std::find_if(packets + offset, packets + length, [&](auto packet) {
            auto stream_id = packetio::packet::signature_stream_id(packet);
            return !stream_id.has_value();
        });
    return std::distance(packets, found);
}

bpf_funcs bpf_no_sig_funcs{
    bpf_no_sig_filter_func, bpf_no_sig_exec_func, bpf_no_sig_find_next_func};

uint16_t bpf_no_sig_and_bpf_jit_filter_func(
    bpf& bpf,
    const packetio::packet::packet_buffer* const packets[],
    const packetio::packet::packet_buffer* results[],
    uint16_t length)
{
    uint32_t mem[BPF_MEMWORDS];
    bpf_ctx_t ctx{.copfuncs = nullptr,
                  .nfuncs = 0,
                  .extwords = BPF_MEMWORDS,
                  .preinited = bpf_ctx_preinited()};
    bpf_args_t args{
        .pkt = nullptr, .wirelen = 0, .buflen = 0, .mem = mem, .arg = nullptr};
    auto filter_func = bpf.get_filter_func();

    auto results_end =
        std::copy_if(packets, packets + length, results, [&](auto packet) {
            auto stream_id = packetio::packet::signature_stream_id(packet);
            if (stream_id.has_value()) return static_cast<uint32_t>(0);
            bpf_arg_init(args, packet);
            return filter_func(&ctx, &args);
        });

    return std::distance(results, results_end);
}

uint16_t bpf_no_sig_and_bpf_jit_exec_func(
    bpf& bpf,
    const packetio::packet::packet_buffer* const packets[],
    uint64_t results[],
    uint16_t length)
{
    uint32_t mem[BPF_MEMWORDS];
    bpf_ctx_t ctx{.copfuncs = nullptr,
                  .nfuncs = 0,
                  .extwords = BPF_MEMWORDS,
                  .preinited = bpf_ctx_preinited()};
    bpf_args_t args{
        .pkt = nullptr, .wirelen = 0, .buflen = 0, .mem = mem, .arg = nullptr};
    auto filter_func = bpf.get_filter_func();

    std::transform(packets, packets + length, results, [&](auto packet) {
        auto stream_id = packetio::packet::signature_stream_id(packet);
        if (stream_id.has_value()) return static_cast<uint32_t>(0);
        bpf_arg_init(args, packet);
        return filter_func(&ctx, &args);
    });

    return length;
}

uint16_t bpf_no_sig_and_bpf_jit_find_next_func(
    bpf& bpf,
    const packetio::packet::packet_buffer* const packets[],
    uint16_t length,
    uint16_t offset)
{
    uint32_t mem[BPF_MEMWORDS];
    bpf_ctx_t ctx{.copfuncs = nullptr,
                  .nfuncs = 0,
                  .extwords = BPF_MEMWORDS,
                  .preinited = bpf_ctx_preinited()};
    bpf_args_t args{
        .pkt = nullptr, .wirelen = 0, .buflen = 0, .mem = mem, .arg = nullptr};
    auto filter_func = bpf.get_filter_func();

    auto found =
        std::find_if(packets + offset, packets + length, [&](auto packet) {
            auto stream_id = packetio::packet::signature_stream_id(packet);
            if (stream_id.has_value()) return static_cast<uint32_t>(0);
            bpf_arg_init(args, packet);
            return filter_func(&ctx, &args);
        });
    return std::distance(packets, found);
}

bpf_funcs bpf_no_sig_and_bpf_jit_funcs{bpf_no_sig_and_bpf_jit_filter_func,
                                       bpf_no_sig_and_bpf_jit_exec_func,
                                       bpf_no_sig_and_bpf_jit_find_next_func};

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

bool bpf_validate_filter(std::string_view filter_str, int link_type)
{
    bpf bpf;
    return bpf.parse(filter_str, link_type);
}

bpf::bpf()
    : m_flags(0)
    , m_funcs(bpf_all_funcs)
{}

bpf::bpf(std::string_view filter_str, int link_type)
    : m_flags(0)
    , m_funcs(bpf_all_funcs)
{
    if (!parse(filter_str, link_type)) {
        std::ostringstream os;
        os << "BPF filter_str " << filter_str << " is not valid";
        throw std::invalid_argument(os.str());
    }
}

bpf::bpf(const bpf_insn* insns, unsigned int len, uint32_t flags)
    : m_flags(0)
    , m_funcs(bpf_all_funcs)
{
    if (!set_prog(insns, len, flags)) {
        throw std::invalid_argument("BPF insns not valid");
    }
}

bool bpf::set_prog(const bpf_insn* insns, unsigned int len, uint32_t flags)
{
    bpf_ctx_t ctx{.copfuncs = nullptr,
                  .nfuncs = 0,
                  .extwords = BPF_MEMWORDS,
                  .preinited = bpf_ctx_preinited()};

    if (!op_bpf_validate_ext(&ctx, insns, len)) {
        OP_LOG(OP_LOG_ERROR, "Unable to validate BPF program");
        return false;
    }

    m_insn.resize(0);
    m_insn.reserve(len);
    std::copy(insns, insns + len, std::back_inserter(m_insn));

    m_jit = bpf_jit(&ctx, insns, len);

    if (!m_jit) {
        OP_LOG(OP_LOG_DEBUG, "Unable to generate BPF JIT code");
        m_funcs = bpf_vm_funcs;
    } else {
        m_funcs = bpf_jit_funcs;
    }
    m_flags = flags;
    return true;
}

bool bpf::parse(std::string_view filter_str, int link_type)
{
    std::unique_ptr<expr> expr;

    try {
        expr = bpf_parse_string(filter_str);
    } catch (const std::runtime_error& e) {
        OP_LOG(OP_LOG_ERROR,
               "Error parsing BPF string %.*s.  %s",
               static_cast<int>(filter_str.length()),
               filter_str.data(),
               e.what());
        return false;
    }

    if (expr && expr->has_special()) {
        std::vector<bpf_insn> bf_insns;
        try {
            expr = bpf_split_special(std::move(expr));
        } catch (const std::runtime_error& e) {
            OP_LOG(OP_LOG_ERROR,
                   "Error splitting BPF expression into special and normal "
                   "expressions %.*s.  %s",
                   static_cast<int>(filter_str.length()),
                   filter_str.data(),
                   e.what());
            return false;
        }
        auto flags = bpf_get_filter_flags(expr.get());
        if ((flags & BPF_FILTER_FLAGS_BPF) == 0) {
            // Handle simple cases without using BPF
            if (flags == BPF_FILTER_FLAGS_SIGNATURE) {
                m_funcs = bpf_sig_funcs;
                m_flags = flags;
                return true;
            }
            if (flags == (BPF_FILTER_FLAGS_SIGNATURE | BPF_FILTER_FLAGS_NOT)) {
                m_funcs = bpf_no_sig_funcs;
                m_flags = flags;
                return true;
            }
        }
        if (auto bexpr = dynamic_cast<binary_logical_expr*>(expr.get())) {
            if (!bexpr->rhs->has_special()) {
                auto lhs_flags = bpf_get_filter_flags(bexpr->lhs.get());
                if (lhs_flags
                        == (BPF_FILTER_FLAGS_SIGNATURE | BPF_FILTER_FLAGS_NOT)
                    && bexpr->op == binary_logical_op::AND) {
                    // Non-signature packets with BPF program is a likely case
                    // where avoiding extra CPU cycles for signature packets
                    // could improve performance, so use optimized version.
                    auto rhs_filter_str = bexpr->rhs->to_string();
                    auto prog = bpf_compile(rhs_filter_str, link_type);
                    if (!prog) {
                        OP_LOG(OP_LOG_ERROR,
                               "Error building BPF program for expression %s",
                               rhs_filter_str.c_str());
                        return false;
                    }
                    if (set_prog(prog->bf_insns, prog->bf_len, flags)) {
                        // If BPF compiles, use specialized functions
                        // Otherwise fallthrough and use mixed BPF program
                        if (m_jit) {
                            m_funcs = bpf_no_sig_and_bpf_jit_funcs;
                            return true;
                        }
                    }
                }
                if (!bpf_build_prog_mixed(bexpr, bf_insns, link_type)) {
                    OP_LOG(OP_LOG_ERROR,
                           "Error building BPF program for mixed case "
                           "expression %.*s",
                           static_cast<int>(filter_str.length()),
                           filter_str.data());
                    return false;
                }
                return set_prog(&bf_insns[0], bf_insns.size(), flags);
            }
        }
        // All special
        if (!bpf_build_prog_all_special(expr.get(), bf_insns)) {
            OP_LOG(
                OP_LOG_ERROR,
                "Error building BPF program for special case expression %.*s",
                static_cast<int>(filter_str.length()),
                filter_str.data());
            return false;
        }
        return set_prog(&bf_insns[0], bf_insns.size(), flags);
    } else {
        auto prog = bpf_compile(filter_str, link_type);
        if (!prog) {
            OP_LOG(OP_LOG_ERROR,
                   "Error building BPF program for expression %.*s",
                   static_cast<int>(filter_str.length()),
                   filter_str.data());
            return false;
        }
        return set_prog(prog->bf_insns, prog->bf_len, 0);
    }
}

} // namespace openperf::packet::bpf