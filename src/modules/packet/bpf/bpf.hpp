#ifndef _OP_PACKET_BPF_HPP_
#define _OP_PACKET_BPF_HPP_

#include <cstdint>
#include <string>
#include <memory>
#include <vector>

#include <pcap.h>
#include "packet/bpf/bsd/bpf.h"

namespace openperf::packetio::packet {
struct packet_buffer;
}

namespace openperf::packet::bpf {

using bpf_program_ptr = std::unique_ptr<bpf_program, void (*)(bpf_program*)>;

/**
 * Compiles a libpcap BPF filter expression string int a BPF program.
 *
 * @return std::unique_ptr for a BPF program if successful or
 *         empty std::unique_ptr if fail.
 */
bpf_program_ptr bpf_compile(std::string_view filter,
                            int link_type = DLT_EN10MB);

/**
 * BPF JIT program smart pointer.
 *
 * Needed to add class because unique_ptr<> doesn't work with function pointers.
 */
class bpf_jit_ptr
{
public:
    bpf_jit_ptr()
        : m_func(nullptr)
    {}

    bpf_jit_ptr(bpfjit_func_t func)
        : m_func(func)
    {}

    bpf_jit_ptr(bpf_jit_ptr&& rhs)
        : m_func(rhs.m_func)
    {
        rhs.m_func = nullptr;
    }

    ~bpf_jit_ptr()
    {
        if (m_func) reset();
    }

    void reset(bpfjit_func_t func = nullptr)
    {
        if (m_func) { op_bpfjit_free_code(m_func); }
        m_func = func;
    }

    bpf_jit_ptr& operator=(bpf_jit_ptr&& rhs)
    {

        m_func = rhs.m_func;
        rhs.m_func = nullptr;
        return *this;
    }

    auto operator*() const { return m_func; }

    operator bool() const { return m_func != nullptr; }

private:
    bpfjit_func_t m_func;
};

/**
 * JIT compiles the BPF program.
 * @return BPF JIT smart pointer if successful, or empty smart pointer if fail.
 */
auto inline bpf_jit(bpf_ctx_t* ctx, const struct bpf_insn* insns, int len)
{
    return bpf_jit_ptr(op_bpfjit_generate_code(ctx, insns, len));
}

/**
 * Validate BPF filter string.
 * @return true if OK, false if error
 */
bool bpf_validate_filter(std::string_view filter_str,
                         int link_type = DLT_EN10MB);

class bpf;

struct bpf_funcs
{
    uint16_t (*m_filter_burst_func)(
        bpf& bpf,
        const openperf::packetio::packet::packet_buffer* const packets[],
        const openperf::packetio::packet::packet_buffer* results[],
        uint16_t length);
    uint16_t (*m_exec_burst_func)(
        bpf& bpf,
        const openperf::packetio::packet::packet_buffer* const packets[],
        uint64_t results[],
        uint16_t length);
    uint16_t (*m_find_next_func)(
        bpf& bpf,
        const openperf::packetio::packet::packet_buffer* const packets[],
        uint16_t length,
        uint16_t offset);
};

/**
 * The bpf class provides a simple interfaces for building and executing
 * BPF programs.
 */
class bpf
{
public:
    bpf();
    bpf(std::string_view filter_str, int link_type = DLT_EN10MB);
    bpf(const bpf_insn* insns, unsigned int len, uint32_t flags = 0);
    bpf(const bpf& bpf) = delete;

    /**
     * Runs the filter program and returns the results in the results array.
     * @param[in] packets The packets to run the filter on
     * @param[out] results The packets which matched the filter
     * @param[in] length The length of the packets and results array
     * @return The number of packets which matche
     */
    uint16_t filter_burst(
        const openperf::packetio::packet::packet_buffer* const packets[],
        const openperf::packetio::packet::packet_buffer* results[],
        uint16_t length)
    {
        return m_funcs.m_filter_burst_func(*this, packets, results, length);
    }

    /**
     * Runs the filter program and returns the results in the results array.
     * @param[in] packets The packets to run the filter on
     * @param[out] results The BPF filter program results
     * @param[in] length The length of the packets and results array
     * @return The number of packets which were processed
     */
    uint16_t
    exec_burst(const openperf::packetio::packet::packet_buffer* const packets[],
               uint64_t results[],
               uint16_t length)
    {
        return m_funcs.m_exec_burst_func(*this, packets, results, length);
    }

    /**
     * Finds the index of the next packet which matches the filter.
     * @param[in] packets The packets to run the filter on
     * @param[in] length The length of the packets and results array
     * @param[in] offset The starting offset
     * @return The index of the first packet which matched the filter
     */
    uint16_t
    find_next(const openperf::packetio::packet::packet_buffer* const packets[],
              uint16_t length,
              uint16_t offset = 0)
    {
        return m_funcs.m_find_next_func(*this, packets, length, offset);
    }

    uint32_t get_filter_flags() const { return m_flags; }

    const std::vector<bpf_insn>& get_prog() const { return m_insn; }
    bpfjit_func_t get_filter_func() const { return *m_jit; }

    bool parse(std::string_view filter_str, int link_type);

private:
    bool set_prog(const bpf_insn* insns, unsigned int len, uint32_t flags);

    uint32_t m_flags;
    std::vector<bpf_insn> m_insn;
    bpf_jit_ptr m_jit;
    bpf_funcs m_funcs;
};

} // namespace openperf::packet::bpf

#endif //_OP_PACKET_BPF_HPP_
