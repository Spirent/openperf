#include <algorithm>
#include <cassert>
#include <numeric>
#include <vector>
#include <map>
#include <memory>

#include "core/op_log.h"
#include "packetio/bpf/bpf.hpp"
#include "packetio/bpf/bpf_build.hpp"

namespace openperf::packetio::bpf {

void bpf_resolve_jmp(std::vector<bpf_insn>& bf_insns,
                     std::map<size_t, bpf_insn_jmp_info>& jmp_map,
                     size_t start,
                     size_t count,
                     bpf_branch_type branch,
                     size_t jmp_offset)
{
    auto found = jmp_map.lower_bound(start);
    while (found != jmp_map.end() && found->first < (start + count)) {
        auto index = found->first;
        auto& tf_cond = found->second;
        if (tf_cond.jt.has_value() && tf_cond.jt.value() == branch) {
            bf_insns[index].jt = jmp_offset - index - 1;
            tf_cond.jt = {};
        }
        if (tf_cond.jf.has_value() && tf_cond.jf.value() == branch) {
            bf_insns[index].jf = jmp_offset - index - 1;
            tf_cond.jf = {};
        }
        if (!tf_cond.jt && !tf_cond.jf) {
            // All jump conditions at this index resolved so entry can be erased
            jmp_map.erase(found++);
        } else {
            ++found;
        }
    }
}

void bpf_invert_jmp(std::map<size_t, bpf_insn_jmp_info>& jmp_map,
                    size_t start,
                    size_t count)
{
    auto found = jmp_map.lower_bound(start);
    while (found != jmp_map.end() && found->first < (start + count)) {
        auto& tf_cond = found->second;
        if (tf_cond.jt.has_value()) {
            tf_cond.jt = bpf_invert_branch_type(*tf_cond.jt);
        }
        if (tf_cond.jf.has_value()) {
            tf_cond.jf = bpf_invert_branch_type(*tf_cond.jf);
        }
        ++found;
    }
}

size_t bpf_find_ret(std::vector<bpf_insn>& bf_insns,
                    size_t offset,
                    bpf_branch_type branch)
{
    auto found =
        std::find_if(bf_insns.begin() + offset, bf_insns.end(), [&](auto& ins) {
            if (branch == bpf_branch_type::PASS) {
                if (ins.code & (BPF_RET | BPF_K) && ins.k != 0) return true;
                return false;
            } else {
                if (ins.code & (BPF_RET | BPF_K) && ins.k == 0) return true;
                return false;
            }
        });

    return std::distance(bf_insns.begin(), found);
}

bool bpf_build_prog_special(const expr* e,
                            std::vector<bpf_insn>& bf_insns,
                            std::map<size_t, bpf_insn_jmp_info>& jmp_map)
{
    if (auto bexpr = dynamic_cast<const binary_logical_expr*>(e)) {
        size_t lhs_block_start = bf_insns.size();
        if (!bpf_build_prog_special(bexpr->lhs.get(), bf_insns, jmp_map))
            return false;
        size_t rhs_block_start = bf_insns.size();
        if (!bpf_build_prog_special(bexpr->rhs.get(), bf_insns, jmp_map))
            return false;
        auto resolved = binary_logical_op_resolves_jmp(bexpr->op);
        bpf_resolve_jmp(bf_insns,
                        jmp_map,
                        lhs_block_start,
                        rhs_block_start - lhs_block_start,
                        resolved,
                        rhs_block_start);
        return true;
    } else if (auto uexpr = dynamic_cast<const unary_logical_expr*>(e)) {
        size_t block_start = bf_insns.size();
        if (!bpf_build_prog_special(uexpr->expr.get(), bf_insns, jmp_map))
            return false;
        size_t block_end = bf_insns.size();
        bpf_invert_jmp(jmp_map, block_start, block_end - block_start);
        return true;
    } else if (auto vexpr = dynamic_cast<const valid_match_expr*>(e)) {
        uint32_t flags = 0;
        if (vexpr->flags
            & static_cast<uint32_t>(valid_match_expr::flag_type::ETH_FCS))
            flags |= BPF_PKTFLAG_FCS_ERROR;
        if (vexpr->flags
            & static_cast<uint32_t>(valid_match_expr::flag_type::TCPUDP_CHKSUM))
            flags |= BPF_PKTFLAG_CHKSUM_ERROR;
        if (vexpr->flags
            & static_cast<uint32_t>(
                valid_match_expr::flag_type::SIGNATURE_PRBS))
            flags |= BPF_PKTFLAG_PRBS_ERROR;
        // Load the packet flags into the A register
        bf_insns.emplace_back(bpf_insn{
            .code = BPF_LD | BPF_MEM, .jt = 0, .jf = 0, .k = BPF_MEM_PKTFLAGS});
        // Binary AND packet flags with flag mask
        bf_insns.emplace_back(bpf_insn{
            .code = BPF_JMP | BPF_JSET | BPF_K, .jt = 0, .jf = 0, .k = flags});
        jmp_map[bf_insns.size() - 1] = bpf_insn_jmp_info{
            .jt = bpf_branch_type::PASS, .jf = bpf_branch_type::FAIL};
        return true;
    } else if (auto sigexpr = dynamic_cast<const signature_match_expr*>(e)) {
        uint32_t flags = BPF_PKTFLAG_SIGNATURE;
        // Load the packet flags into the A register
        bf_insns.emplace_back(bpf_insn{
            .code = BPF_LD | BPF_MEM, .jt = 0, .jf = 0, .k = BPF_MEM_PKTFLAGS});
        // Binary AND packet flags with flag mask
        bf_insns.emplace_back(bpf_insn{
            .code = BPF_JMP | BPF_JSET | BPF_K, .jt = 0, .jf = 0, .k = flags});
        if (!sigexpr->stream_id.has_value()) {
            jmp_map[bf_insns.size() - 1] = bpf_insn_jmp_info{
                .jt = bpf_branch_type::PASS, .jf = bpf_branch_type::FAIL};
        } else {
            jmp_map[bf_insns.size() - 1] =
                bpf_insn_jmp_info{.jt = {}, .jf = bpf_branch_type::FAIL};

            // Load the stream ID into the A register
            bf_insns.emplace_back(bpf_insn{.code = BPF_LD | BPF_MEM,
                                           .jt = 0,
                                           .jf = 0,
                                           .k = BPF_MEM_STREAM_ID});
            auto val = sigexpr->stream_id.value();
            if (val.start == val.end) {
                auto index = bf_insns.size();
                bf_insns.emplace_back(
                    bpf_insn{.code = BPF_JMP | BPF_JEQ | BPF_K,
                             .jt = 0,
                             .jf = 0,
                             .k = val.start});
                jmp_map[index] = bpf_insn_jmp_info{.jt = bpf_branch_type::PASS,
                                                   .jf = bpf_branch_type::FAIL};
            } else {
                auto index = bf_insns.size();
                bf_insns.emplace_back(
                    bpf_insn{.code = BPF_JMP | BPF_JGE | BPF_K,
                             .jt = 0,
                             .jf = 0,
                             .k = val.start});
                jmp_map[index] =
                    bpf_insn_jmp_info{.jt = {}, .jf = bpf_branch_type::FAIL};
                index = bf_insns.size();
                bf_insns.emplace_back(
                    bpf_insn{.code = BPF_JMP | BPF_JGT | BPF_K,
                             .jt = 0,
                             .jf = 0,
                             .k = val.end});
                jmp_map[index] = bpf_insn_jmp_info{.jt = bpf_branch_type::FAIL,
                                                   .jf = bpf_branch_type::PASS};
            }
        }
        return true;
    }
    return false;
}

bool bpf_build_prog_all_special(const expr* e, std::vector<bpf_insn>& bf_insns)
{
    std::map<size_t, bpf_insn_jmp_info> jmp_map;

    auto block_start = bf_insns.size();
    if (!bpf_build_prog_special(e, bf_insns, jmp_map)) return false;
    auto block_end = bf_insns.size();

    // Add BPF_RET entries for PASS and FAIL cases
    auto pass_index = bf_insns.size();
    bf_insns.emplace_back(
        bpf_insn{.code = BPF_RET | BPF_K, .jt = 0, .jf = 0, .k = 0x00040000});
    auto fail_index = bf_insns.size();
    bf_insns.emplace_back(
        bpf_insn{.code = BPF_RET | BPF_K, .jt = 0, .jf = 0, .k = 0x00000000});

    bpf_resolve_jmp(bf_insns,
                    jmp_map,
                    block_start,
                    block_end,
                    bpf_branch_type::PASS,
                    pass_index);

    bpf_resolve_jmp(bf_insns,
                    jmp_map,
                    block_start,
                    block_end,
                    bpf_branch_type::FAIL,
                    fail_index);

    return true;
}

bool bpf_build_prog_mixed(const binary_logical_expr* e,
                          std::vector<bpf_insn>& bf_insns,
                          int link_type)
{
    // lhs side is special and rhs is normal libpcap
    std::map<size_t, bpf_insn_jmp_info> jmp_map;

    // Add instructions for special case expressions
    auto lhs_block_start = bf_insns.size();
    if (!bpf_build_prog_special(e->lhs.get(), bf_insns, jmp_map)) return false;
    auto rhs_block_start = bf_insns.size();

    // Add instructions for libpcap expressions
    auto libpcap_filter_str = e->rhs->to_string();
    auto prog = bpf_compile(libpcap_filter_str, link_type);
    if (!prog) return false;
    std::copy_n(prog->bf_insns, prog->bf_len, std::back_inserter(bf_insns));

    // Resolve jmp in lhs block which jump to start of rhs block
    auto resolved = binary_logical_op_resolves_jmp(e->op);
    bpf_resolve_jmp(bf_insns,
                    jmp_map,
                    lhs_block_start,
                    rhs_block_start - lhs_block_start,
                    resolved,
                    rhs_block_start);

    // Resolve other jmp instructions in lhs block which jump to a return result
    resolved = bpf_invert_branch_type(resolved);
    auto ret_index = bpf_find_ret(bf_insns, rhs_block_start, resolved);
    if (ret_index > bf_insns.size()) {
        // Normally a matching BPF_RET should be found, so this case
        // should never happen...
        uint32_t ret_val = (resolved == bpf_branch_type::PASS) ? 0x00040000 : 0;
        bf_insns.emplace_back(
            bpf_insn{.code = BPF_RET | BPF_K, .jt = 0, .jf = 0, .k = ret_val});
        ret_index = bf_insns.size() - 1;
    }
    bpf_resolve_jmp(bf_insns,
                    jmp_map,
                    lhs_block_start,
                    rhs_block_start - lhs_block_start,
                    resolved,
                    ret_index);
    return true;
}

uint32_t bpf_get_filter_flags(const expr* e)
{
    if (auto bexpr = dynamic_cast<const binary_logical_expr*>(e)) {
        auto lhs_flags = bpf_get_filter_flags(bexpr->lhs.get());
        auto rhs_flags = bpf_get_filter_flags(bexpr->rhs.get());
        auto op_flags = (bexpr->op == binary_logical_op::AND)
                            ? BPF_FILTER_FLAGS_AND
                            : BPF_FILTER_FLAGS_OR;
        auto flags = lhs_flags | rhs_flags | op_flags;
        if ((flags & (BPF_FILTER_FLAGS_AND | BPF_FILTER_FLAGS_OR))
            == ((BPF_FILTER_FLAGS_AND | BPF_FILTER_FLAGS_OR))) {
            // Mixed and/or logical operators requires BPF program
            flags |= BPF_FILTER_FLAGS_BPF;
        }
        return flags;
    } else if (auto uexpr = dynamic_cast<const unary_logical_expr*>(e)) {
        auto flags = bpf_get_filter_flags(uexpr->expr.get());
        if (uexpr->op == unary_logical_op::NOT) {
            if (flags & BPF_FILTER_FLAGS_NOT) {
                // Double not operator requires BPF program
                flags |= BPF_FILTER_FLAGS_BPF;
            }
            flags |= BPF_FILTER_FLAGS_NOT;
        }
        return flags;
    } else if (auto* vexpr = dynamic_cast<const valid_match_expr*>(e)) {
        uint32_t flags = 0;
        if (vexpr->flags
            & static_cast<uint32_t>(valid_match_expr::flag_type::ETH_FCS))
            flags |= BPF_FILTER_FLAGS_FCS_ERROR;
        if (vexpr->flags
            & static_cast<uint32_t>(valid_match_expr::flag_type::TCPUDP_CHKSUM))
            flags |= BPF_FILTER_FLAGS_CHKSUM_ERROR;
        if (vexpr->flags
            & static_cast<uint32_t>(
                valid_match_expr::flag_type::SIGNATURE_PRBS))
            flags |= BPF_FILTER_FLAGS_PRBS_ERROR;
        return flags;
    } else if (auto* sigexpr = dynamic_cast<const signature_match_expr*>(e)) {
        if (sigexpr->stream_id.has_value()) {
            return BPF_FILTER_FLAGS_SIGNATURE
                   | BPF_FILTER_FLAGS_SIGNATURE_STREAM_ID;
        }
        return BPF_FILTER_FLAGS_SIGNATURE;
    }

    // libpcap expression requires BPF program
    return BPF_FILTER_FLAGS_BPF;
}

} // namespace openperf::packetio::bpf