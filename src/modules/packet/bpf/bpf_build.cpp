#include <algorithm>
#include <numeric>
#include <vector>
#include <map>
#include <memory>

#include "core/op_log.h"
#include "packet/bpf/bpf.hpp"
#include "packet/bpf/bpf_build.hpp"

namespace openperf::packet::bpf {

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

class build_prog_speical_visitor : public expr_visitor
{
public:
    build_prog_speical_visitor(std::vector<bpf_insn>& bf_insns,
                               std::map<size_t, bpf_insn_jmp_info>& jmp_map)
        : m_bf_insns(bf_insns)
        , m_jmp_map(jmp_map)
    {}

    void visit(const generic_match_expr&) override
    {
        throw std::runtime_error("Unexpected generic_match_expr");
    }

    void visit(const valid_match_expr& expr) override
    {
        uint32_t flags = 0;
        if (expr.flags
            & static_cast<uint32_t>(valid_match_expr::flag_type::ETH_FCS))
            flags |= BPF_PKTFLAG_FCS_ERROR;
        if (expr.flags
            & static_cast<uint32_t>(valid_match_expr::flag_type::IP_CHKSUM))
            flags |= BPF_PKTFLAG_IP_CHKSUM_ERROR;
        if (expr.flags
            & static_cast<uint32_t>(valid_match_expr::flag_type::TCP_CHKSUM))
            flags |= BPF_PKTFLAG_TCP_CHKSUM_ERROR;
        if (expr.flags
            & static_cast<uint32_t>(valid_match_expr::flag_type::UDP_CHKSUM))
            flags |= BPF_PKTFLAG_UDP_CHKSUM_ERROR;
        if (expr.flags
            & static_cast<uint32_t>(valid_match_expr::flag_type::ICMP_CHKSUM))
            flags |= BPF_PKTFLAG_ICMP_CHKSUM_ERROR;
        if (expr.flags
            & static_cast<uint32_t>(
                valid_match_expr::flag_type::SIGNATURE_PRBS))
            flags |= BPF_PKTFLAG_PRBS_ERROR;
        // Load the packet flags into the A register
        m_bf_insns.emplace_back(bpf_insn{
            .code = BPF_LD | BPF_MEM, .jt = 0, .jf = 0, .k = BPF_MEM_PKTFLAGS});
        // Binary AND packet flags with flag mask
        m_bf_insns.emplace_back(bpf_insn{
            .code = BPF_JMP | BPF_JSET | BPF_K, .jt = 0, .jf = 0, .k = flags});
        m_jmp_map[m_bf_insns.size() - 1] = bpf_insn_jmp_info{
            .jt = bpf_branch_type::PASS, .jf = bpf_branch_type::FAIL};
    }

    void visit(const signature_match_expr& expr) override
    {
        uint32_t flags = BPF_PKTFLAG_SIGNATURE;
        // Load the packet flags into the A register
        m_bf_insns.emplace_back(bpf_insn{
            .code = BPF_LD | BPF_MEM, .jt = 0, .jf = 0, .k = BPF_MEM_PKTFLAGS});
        // Binary AND packet flags with flag mask
        m_bf_insns.emplace_back(bpf_insn{
            .code = BPF_JMP | BPF_JSET | BPF_K, .jt = 0, .jf = 0, .k = flags});
        if (!expr.stream_id.has_value()) {
            m_jmp_map[m_bf_insns.size() - 1] = bpf_insn_jmp_info{
                .jt = bpf_branch_type::PASS, .jf = bpf_branch_type::FAIL};
        } else {
            m_jmp_map[m_bf_insns.size() - 1] =
                bpf_insn_jmp_info{.jt = {}, .jf = bpf_branch_type::FAIL};

            // Load the stream ID into the A register
            m_bf_insns.emplace_back(bpf_insn{.code = BPF_LD | BPF_MEM,
                                             .jt = 0,
                                             .jf = 0,
                                             .k = BPF_MEM_STREAM_ID});
            auto val = expr.stream_id.value();
            if (val.start == val.end) {
                auto index = m_bf_insns.size();
                m_bf_insns.emplace_back(
                    bpf_insn{.code = BPF_JMP | BPF_JEQ | BPF_K,
                             .jt = 0,
                             .jf = 0,
                             .k = val.start});
                m_jmp_map[index] = bpf_insn_jmp_info{
                    .jt = bpf_branch_type::PASS, .jf = bpf_branch_type::FAIL};
            } else {
                auto index = m_bf_insns.size();
                m_bf_insns.emplace_back(
                    bpf_insn{.code = BPF_JMP | BPF_JGE | BPF_K,
                             .jt = 0,
                             .jf = 0,
                             .k = val.start});
                m_jmp_map[index] =
                    bpf_insn_jmp_info{.jt = {}, .jf = bpf_branch_type::FAIL};
                index = m_bf_insns.size();
                m_bf_insns.emplace_back(
                    bpf_insn{.code = BPF_JMP | BPF_JGT | BPF_K,
                             .jt = 0,
                             .jf = 0,
                             .k = val.end});
                m_jmp_map[index] = bpf_insn_jmp_info{
                    .jt = bpf_branch_type::FAIL, .jf = bpf_branch_type::PASS};
            }
        }
    }

    void visit(const unary_logical_expr& expr) override
    {
        size_t block_start = m_bf_insns.size();
        expr.expr->accept(*this);
        size_t block_end = m_bf_insns.size();
        bpf_invert_jmp(m_jmp_map, block_start, block_end - block_start);
    }

    void visit(const binary_logical_expr& expr) override
    {
        size_t lhs_block_start = m_bf_insns.size();
        expr.lhs->accept(*this);
        size_t rhs_block_start = m_bf_insns.size();
        expr.rhs->accept(*this);
        auto resolved = binary_logical_op_resolves_jmp(expr.op);
        bpf_resolve_jmp(m_bf_insns,
                        m_jmp_map,
                        lhs_block_start,
                        rhs_block_start - lhs_block_start,
                        resolved,
                        rhs_block_start);
    }

private:
    std::vector<bpf_insn>& m_bf_insns;
    std::map<size_t, bpf_insn_jmp_info>& m_jmp_map;
};

bool bpf_build_prog_special(const expr* e,
                            std::vector<bpf_insn>& bf_insns,
                            std::map<size_t, bpf_insn_jmp_info>& jmp_map)
{
    build_prog_speical_visitor visitor(bf_insns, jmp_map);
    try {
        e->accept(visitor);
    } catch (const std::runtime_error& error) {
        return false;
    }
    return true;
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
    if (ret_index >= bf_insns.size()) {
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

class bpf_get_filter_flags_visitor : public expr_visitor
{
public:
    bpf_get_filter_flags_visitor(uint32_t flags = 0)
        : m_flags(flags)
    {}

    uint32_t get_flags() const { return m_flags; }

    void visit(const generic_match_expr&) override
    {
        m_flags |= BPF_FILTER_FLAGS_BPF;
    }

    void visit(const valid_match_expr& expr) override
    {
        if (expr.flags
            & static_cast<uint32_t>(valid_match_expr::flag_type::ETH_FCS))
            m_flags |= BPF_FILTER_FLAGS_FCS_ERROR;
        if (expr.flags
            & static_cast<uint32_t>(valid_match_expr::flag_type::IP_CHKSUM))
            m_flags |= BPF_FILTER_FLAGS_IP_CHKSUM_ERROR;
        if (expr.flags
            & static_cast<uint32_t>(valid_match_expr::flag_type::TCP_CHKSUM))
            m_flags |= BPF_FILTER_FLAGS_TCP_CHKSUM_ERROR;
        if (expr.flags
            & static_cast<uint32_t>(valid_match_expr::flag_type::UDP_CHKSUM))
            m_flags |= BPF_FILTER_FLAGS_UDP_CHKSUM_ERROR;
        if (expr.flags
            & static_cast<uint32_t>(valid_match_expr::flag_type::ICMP_CHKSUM))
            m_flags |= BPF_FILTER_FLAGS_ICMP_CHKSUM_ERROR;
        if (expr.flags
            & static_cast<uint32_t>(
                valid_match_expr::flag_type::SIGNATURE_PRBS)) {
            m_flags |= BPF_FILTER_FLAGS_SIGNATURE | BPF_FILTER_FLAGS_PRBS_ERROR;
        }
    }

    void visit(const signature_match_expr& expr) override
    {
        m_flags |= BPF_FILTER_FLAGS_SIGNATURE;
        if (expr.stream_id.has_value()) {
            m_flags |= BPF_FILTER_FLAGS_SIGNATURE_STREAM_ID;
        }
    }

    void visit(const unary_logical_expr& expr) override
    {
        if (expr.op == unary_logical_op::NOT) {
            if (m_flags & BPF_FILTER_FLAGS_NOT) {
                // Double not operator requires BPF program
                m_flags |= BPF_FILTER_FLAGS_BPF;
            }
            m_flags |= BPF_FILTER_FLAGS_NOT;
            expr.expr->accept(*this);
        }
    }

    void visit(const binary_logical_expr& expr) override
    {
        m_flags |= (expr.op == binary_logical_op::AND) ? BPF_FILTER_FLAGS_AND
                                                       : BPF_FILTER_FLAGS_OR;
        if ((m_flags & (BPF_FILTER_FLAGS_AND | BPF_FILTER_FLAGS_OR))
            == ((BPF_FILTER_FLAGS_AND | BPF_FILTER_FLAGS_OR))) {
            // Mixed and/or logical operators requires BPF program
            m_flags |= BPF_FILTER_FLAGS_BPF;
        }
        expr.lhs->accept(*this);
        expr.rhs->accept(*this);
    }

private:
    uint32_t m_flags;
};

uint32_t bpf_get_filter_flags(const expr* e)
{
    bpf_get_filter_flags_visitor visitor;
    e->accept(visitor);
    return visitor.get_flags();
}

} // namespace openperf::packet::bpf