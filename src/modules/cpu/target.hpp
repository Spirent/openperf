#ifndef _OP_CPU_TARGET_HPP_
#define _OP_CPU_TARGET_HPP_

#include <cinttypes>
#include "cpu/common.hpp"

namespace openperf::cpu::internal {

class target
{
protected:
    cpu::data_size m_data_size;
    cpu::operation m_operation;

public:
    target(cpu::data_size size, cpu::operation op)
        : m_data_size(size), m_operation(op)
        {}
    virtual ~target() = default;
    virtual uint64_t operation() const = 0;

    inline uint64_t operator()() const { return operation(); }
};

} // namespace openperf::cpu::internal

#endif // _OP_CPU_TARGET_HPP_
