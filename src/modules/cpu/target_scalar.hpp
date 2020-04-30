#ifndef _OP_CPU_TARGET_SCALAR_HPP_
#define _OP_CPU_TARGET_SCALAR_HPP_

#include "cpu/target.hpp"

namespace openperf::cpu::internal {

class target_scalar
    : public target
{
public:
    using target::target;
    ~target_scalar() override = default;

    uint64_t operation() const override {
        int n = 90;
        if (n <= 1) return 1;

        uint64_t f = 1;
        uint64_t f_prev = 1;

        for (int i = 2; i < n; ++i) {
            uint64_t tmp = f;
            f += f_prev;
            f_prev = tmp;
        }

        return n;
    }
};

} // namespace openperf::cpu::internal

#endif // _OP_CPU_TARGET_SCALAR_HPP_
