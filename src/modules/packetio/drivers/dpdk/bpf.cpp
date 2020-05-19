#include <algorithm>
#include <cassert>
#include <numeric>
#include <set>
#include <vector>
#include <algorithm>
#include <memory>

// DPDK BPF code is still part of the experimental API
#define ALLOW_EXPERIMENTAL_API

#include "packetio/drivers/dpdk/dpdk.h"
#include "packetio/drivers/dpdk/bpf.hpp"
#include "core/op_log.h"

#include <rte_bpf.h>
#include <pcap.h>

namespace openperf::packetio::dpdk {

uint16_t bpf_no_filter_func(bpf& bpf,
                            rte_mbuf* packets[],
                            uint64_t results[],
                            uint16_t length)
{
    std::fill(results, results + length, 0);
    return length;
}

uint16_t bpf_jit_filter_func(bpf& bpf,
                             rte_mbuf* packets[],
                             uint64_t results[],
                             uint16_t length)
{
    for (uint16_t i = 0; i != length; ++i) {
        auto ctx = rte_pktmbuf_mtod(packets[i], void*);
        results[i] = bpf.m_bpf_jit_func(ctx);
    }
    return length;
}

uint16_t bpf_vm_filter_func(bpf& bpf,
                            rte_mbuf* packets[],
                            uint64_t results[],
                            uint16_t length)
{
    void* ctx[length];

    for (uint16_t i = 0; i != length; ++i) {
        ctx[i] = rte_pktmbuf_mtod(packets[i], void*);
    }

    return rte_bpf_exec_burst(bpf.m_bpf, ctx, results, length);
}

bpf::bpf()
    : m_bpf(nullptr)
    , m_bpf_exec_func(bpf_no_filter_func)
{}

bpf::bpf(std::string_view filter_str, data_link_type link_type)
    : m_bpf(nullptr)
    , m_bpf_exec_func(bpf_no_filter_func)
{
    if (!parse(filter_str, link_type)) {
        throw std::invalid_argument("filter_str is not valid");
    }
}

bpf::~bpf()
{
    if (m_bpf) {
        rte_bpf_destroy(m_bpf);
        m_bpf = nullptr;
    }
}

int to_pcap_linktype(bpf::data_link_type link_type)
{
    switch (link_type) {
    case bpf::data_link_type::ETHERNET:
        return DLT_EN10MB;
    case bpf::data_link_type::IPV4:
        return DLT_IPV4;
    case bpf::data_link_type::IPV6:
        return DLT_IPV6;
    }
}

#define BPF_ALU32_REG(OP, DST, SRC)                                            \
    ((struct ebpf_insn){.code = BPF_ALU | BPF_OP(OP) | BPF_X,                  \
                        .dst_reg = DST,                                        \
                        .src_reg = SRC,                                        \
                        .off = 0,                                              \
                        .imm = 0})

#define BPF_MOV64_REG(DST, SRC)                                                \
    ((struct ebpf_insn){.code = EBPF_ALU64 | EBPF_MOV | BPF_X,                 \
                        .dst_reg = DST,                                        \
                        .src_reg = SRC,                                        \
                        .off = 0,                                              \
                        .imm = 0})

#define BPF_RAW_INSN(CODE, DST, SRC, OFF, IMM)                                 \
    ((struct ebpf_insn){.code = uint8_t(CODE),                                 \
                        .dst_reg = DST,                                        \
                        .src_reg = SRC,                                        \
                        .off = OFF,                                            \
                        .imm = int32_t(IMM)})

#define BPF_EXIT_INSN()                                                        \
    ((struct ebpf_insn){.code = BPF_JMP | BPF_EXIT,                            \
                        .dst_reg = 0,                                          \
                        .src_reg = 0,                                          \
                        .off = 0,                                              \
                        .imm = 0})

#define BPF_REG_A EBPF_REG_0
#define BPF_REG_X EBPF_REG_7
#define BPF_REG_CTX EBPF_REG_6

#define SKF_AD_OFF (-0x1000)
#define SKF_AD_MAX (64)

bool convert_bpf_extensions(bpf_insn& ins, std::vector<ebpf_insn>& ebpf_ins)
{
    if (ins.k >= (uint32_t)SKF_AD_OFF
        && ins.k <= (uint32_t)(SKF_AD_OFF + SKF_AD_MAX)) {
        OP_LOG(OP_LOG_ERROR,
               "SKF_* extensions are currently not supported!!!! k=%#x",
               ins.k);
        return true;
    }
    return false;
}

bool bpf::parse(std::string_view& filter_str, data_link_type link_type)
{
    struct bpf_program prog
    {};

    // Use libpcap to parse the filter string
    std::string str(filter_str);
    if (pcap_compile_nopcap(0xffff,
                            to_pcap_linktype(link_type),
                            &prog,
                            const_cast<char*>(str.c_str()),
                            1,
                            UINT32_MAX)
        < 0) {
        OP_LOG(OP_LOG_ERROR, "Unable to parse BPF string %s", str.c_str());
        return false;
    }

    // Make sure to free up the BPF prog when exiting scope
    auto prog_scope_dtor =
        std::unique_ptr<bpf_program, decltype(&pcap_freecode)>(&prog,
                                                               pcap_freecode);

    // Convert the cBPF to eBPF
    // This code was modeled after the Linux kernel cbpf to ebpf conversion code
    // in bpf_convert_filter() which use skbuf so it is a bit different.
    std::vector<ebpf_insn> ebpf_ins;

    std::vector<int> bpf_offset_table;
    bool completed = false;

    bpf_offset_table.resize(prog.bf_len);
    ebpf_ins.reserve(
        prog.bf_len); // ebpf is going to be at least as large as cbpf

    while (!completed) {
        bool jump_target_missing = false;

        ebpf_ins.resize(0);
        // Caller loads these registers
        //   EBPF_REG_1  = BPF context
        //   EBPF_REG_10 = BPF stack size

        // Classic BPF expects register A and X to be cleared, so need to clear
        // them
        ebpf_ins.emplace_back(BPF_ALU32_REG(BPF_XOR, BPF_REG_A, BPF_REG_A));
        ebpf_ins.emplace_back(BPF_ALU32_REG(BPF_XOR, BPF_REG_X, BPF_REG_X));
        // Store the context in special register
        // Kernel does this. Not sure if we need to or not...
        ebpf_ins.emplace_back(BPF_MOV64_REG(BPF_REG_CTX, EBPF_REG_1));

        for (u_int i = 0; i < prog.bf_len; ++i) {
            // Keep track of where each instruction in the original bpf maps to
            // in the new bpf This is used by the jump instructions to calculate
            // jump point
            //
            bpf_offset_table[i] = ebpf_ins.size() - 1;
            auto& ins = prog.bf_insns[i];
            switch (ins.code) {
            case BPF_ALU | BPF_ADD | BPF_X:
            case BPF_ALU | BPF_ADD | BPF_K:
            case BPF_ALU | BPF_SUB | BPF_X:
            case BPF_ALU | BPF_SUB | BPF_K:
            case BPF_ALU | BPF_AND | BPF_X:
            case BPF_ALU | BPF_AND | BPF_K:
            case BPF_ALU | BPF_OR | BPF_X:
            case BPF_ALU | BPF_OR | BPF_K:
            case BPF_ALU | BPF_LSH | BPF_X:
            case BPF_ALU | BPF_LSH | BPF_K:
            case BPF_ALU | BPF_RSH | BPF_X:
            case BPF_ALU | BPF_RSH | BPF_K:
            case BPF_ALU | BPF_XOR | BPF_X:
            case BPF_ALU | BPF_XOR | BPF_K:
            case BPF_ALU | BPF_MUL | BPF_X:
            case BPF_ALU | BPF_MUL | BPF_K:
            case BPF_ALU | BPF_DIV | BPF_X:
            case BPF_ALU | BPF_DIV | BPF_K:
            case BPF_ALU | BPF_MOD | BPF_X:
            case BPF_ALU | BPF_MOD | BPF_K:
            case BPF_ALU | BPF_NEG:
            case BPF_LD | BPF_ABS | BPF_W:
            case BPF_LD | BPF_ABS | BPF_H:
            case BPF_LD | BPF_ABS | BPF_B:
            case BPF_LD | BPF_IND | BPF_W:
            case BPF_LD | BPF_IND | BPF_H:
            case BPF_LD | BPF_IND | BPF_B:
                /* Check BPF extensions (e.g. SKF_*) */
                if (BPF_CLASS(ins.code) == BPF_LD
                    && BPF_MODE(ins.code) == BPF_ABS
                    && convert_bpf_extensions(ins, ebpf_ins))
                    break;
                ebpf_ins.emplace_back(
                    BPF_RAW_INSN(ins.code, BPF_REG_A, BPF_REG_X, 0, ins.k));
                break;
            default:
                OP_LOG(OP_LOG_ERROR, "Unsupported BPF code %#x", ins.code);
                return false;
            }
            // ebpf_ins.emplace_back();
        }

        completed = !jump_target_missing;
    }
    rte_bpf_prm prm{};
    prm.ins = &ebpf_ins[0];
    prm.nb_ins = ebpf_ins.size();
    m_bpf = rte_bpf_load(&prm);
    if (m_bpf == nullptr) {
        OP_LOG(OP_LOG_ERROR, "Unable to load BPF");
        return false;
    }

    rte_bpf_jit jit{};
    if (rte_bpf_get_jit(m_bpf, &jit) != 0) {
        OP_LOG(OP_LOG_WARNING, "Unable to JIT BPF");
    } else {
        m_bpf_jit_func = jit.func;
    }

    return true;
}

} // namespace openperf::packetio::dpdk