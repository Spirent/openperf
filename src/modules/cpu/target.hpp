#ifndef _OP_CPU_TARGET_HPP_
#define _OP_CPU_TARGET_HPP_

#include <cinttypes>
#include "common.hpp"

namespace openperf::cpu::internal {

class target
{
public:
    target() = default;
    virtual ~target() = default;
    virtual uint64_t operation() const = 0;

    uint64_t operator()() const { return operation(); }
};

} // namespace openperf::cpu::internal

#endif // _OP_CPU_TARGET_HPP_
