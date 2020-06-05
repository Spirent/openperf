#ifndef _OP_PACKETIO_BPF_HPP_
#define _OP_PACKETIO_BPF_HPP_

#include <cstdint>
#include <string>

#include <pcap.h>
#include "packetio/bpf/bsd/bpf.h"

namespace openperf::packetio::packet {
struct packet_buffer;
}

namespace openperf::packetio::bpf {

using bpf_program_ptr = std::unique_ptr<bpf_program, void (*)(bpf_program*)>;

bpf_program_ptr bpf_compile(std::string_view filter,
                            int link_type = DLT_EN10MB);

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

auto inline bpf_jit(const struct bpf_insn* insns, int len)
{
    return bpf_jit_ptr(op_bpfjit_generate_code(nullptr, insns, len));
}

class bpf
{
public:
    bpf();
    bpf(std::string_view filter_str, int link_type = DLT_EN10MB);
    bpf(const bpf_insn* insns, unsigned int len);
    bpf(const bpf& bpf) = delete;

    uint16_t
    exec_burst(const openperf::packetio::packet::packet_buffer* const packets[],
               uint64_t results[],
               uint16_t length)
    {
        return m_exec_burst_func(*this, packets, results, length);
    }

    uint16_t
    find_next(const openperf::packetio::packet::packet_buffer* const packets[],
              uint16_t length,
              uint16_t offset = 0)
    {
        return m_find_next_func(*this, packets, length, offset);
    }

    const std::vector<bpf_insn>& get_prog() const { return m_insn; }
    bpfjit_func_t get_filter_func() const { return *m_jit; }

private:
    bool set_prog(const bpf_insn* insns, unsigned int len);
    bool parse(std::string_view filter_str, int link_type);

    std::vector<bpf_insn> m_insn;
    bpf_jit_ptr m_jit;

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

} // namespace openperf::packetio::bpf

#endif //_OP_PACKETIO_BPF_HPP_
