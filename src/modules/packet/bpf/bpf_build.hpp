#ifndef _OP_PACKET_BPF_BUILD_HPP_
#define _OP_PACKET_BPF_BUILD_HPP_

#include <optional>
#include <map>
#include "packet/bpf/bpf_parse.hpp"

struct bpf_insn;

namespace openperf::packet::bpf {

constexpr int BPF_MEM_PKTFLAGS = 0;
constexpr int BPF_MEM_STREAM_ID = 1;

constexpr uint32_t BPF_PKTFLAG_FCS_ERROR = 0x01;
constexpr uint32_t BPF_PKTFLAG_IP_CHKSUM_ERROR = 0x02;
constexpr uint32_t BPF_PKTFLAG_TCP_CHKSUM_ERROR = 0x04;
constexpr uint32_t BPF_PKTFLAG_UDP_CHKSUM_ERROR = 0x08;
constexpr uint32_t BPF_PKTFLAG_ICMP_CHKSUM_ERROR = 0x10;
constexpr uint32_t BPF_PKTFLAG_PRBS_ERROR = 0x20;
constexpr uint32_t BPF_PKTFLAG_SIGNATURE = 0x40;

constexpr uint32_t BPF_FILTER_FLAGS_FCS_ERROR = 0x01;
constexpr uint32_t BPF_FILTER_FLAGS_IP_CHKSUM_ERROR = 0x02;
constexpr uint32_t BPF_FILTER_FLAGS_TCP_CHKSUM_ERROR = 0x04;
constexpr uint32_t BPF_FILTER_FLAGS_UDP_CHKSUM_ERROR = 0x08;
constexpr uint32_t BPF_FILTER_FLAGS_ICMP_CHKSUM_ERROR = 0x10;
constexpr uint32_t BPF_FILTER_FLAGS_PRBS_ERROR = 0x20;
constexpr uint32_t BPF_FILTER_FLAGS_SIGNATURE = 0x40;
constexpr uint32_t BPF_FILTER_FLAGS_SIGNATURE_STREAM_ID = 0x100;
constexpr uint32_t BPF_FILTER_FLAGS_AND = 0x200;
constexpr uint32_t BPF_FILTER_FLAGS_OR = 0x400;
constexpr uint32_t BPF_FILTER_FLAGS_NOT = 0x800;
constexpr uint32_t BPF_FILTER_FLAGS_BPF = 0x80000000;

enum class bpf_branch_type { PASS, FAIL };

struct bpf_insn_jmp_info
{
    std::optional<bpf_branch_type> jt;
    std::optional<bpf_branch_type> jf;
};

constexpr bpf_branch_type binary_logical_op_resolves_jmp(binary_logical_op op)
{
    // AND operator, PASS cases in 1st block jump to start of 2nd block
    // OR operator, FAIL cases in 1st block jump to start of 2nd block
    return (op == binary_logical_op::AND) ? bpf_branch_type::PASS
                                          : bpf_branch_type::FAIL;
}

constexpr bpf_branch_type bpf_invert_branch_type(bpf_branch_type result)
{
    return (result == bpf_branch_type::PASS ? bpf_branch_type::FAIL
                                            : bpf_branch_type::PASS);
}

void bpf_resolve_jmp(std::vector<bpf_insn>& bf_insns,
                     std::map<size_t, bpf_insn_jmp_info>& jmp_map,
                     size_t start,
                     size_t count,
                     bpf_branch_type result,
                     size_t jmp_offset);

void bpf_invert_jmp(std::map<size_t, bpf_insn_jmp_info>& jmp_map,
                    size_t start,
                    size_t count);

size_t bpf_find_ret(std::vector<bpf_insn>& bf_insns,
                    size_t offset,
                    bpf_branch_type branch);

bool bpf_build_prog_special(const expr* e,
                            std::vector<bpf_insn>& bf_insns,
                            std::map<size_t, bpf_insn_jmp_info>& jmp_map);

bool bpf_build_prog_all_special(const expr* e, std::vector<bpf_insn>& bf_insns);

bool bpf_build_prog_mixed(const binary_logical_expr* e,
                          std::vector<bpf_insn>& bf_insns,
                          int link_type);

uint32_t bpf_get_filter_flags(const expr* e);

} // namespace openperf::packet::bpf

#endif // _OP_PACKET_BPF_BUILD_HPP_