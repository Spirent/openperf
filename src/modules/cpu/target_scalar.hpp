#ifndef _OP_CPU_TARGET_SCALAR_HPP_
#define _OP_CPU_TARGET_SCALAR_HPP_

#include "cpu/target.hpp"
#include <iostream>
#include <vector>
#include <algorithm>

namespace openperf::cpu::internal {

class target_scalar
    : public target
{
public:
    using target::target;
    ~target_scalar() override = default;

    uint64_t operation() const override {
        switch (m_data_type)
        {
        case cpu::data_type::INT32:
            return operation<uint32_t>();
        case cpu::data_type::INT64:
            return operation<uint64_t>();
        case cpu::data_type::FLOAT32:
            return operation<float>();
        case cpu::data_type::FLOAT64:
            return operation<double>();
        }
    }

private:
    template<class T>
    uint64_t operation() const {
        constexpr size_t size = 1000;

        std::vector<T> v1(size), v2(size), v3(size);

        std::generate(v1.begin(), v1.end(), std::rand);
        std::generate(v2.begin(), v2.end(), std::rand);
        std::generate(v3.begin(), v3.end(), std::rand);

        T result;
        for (size_t i = 0; i < size; ++i) {
            result = v1[i] + v2[i] * v3[i] / v1[i];
        }

        return size;
    }
};

} // namespace openperf::cpu::internal

#endif // _OP_CPU_TARGET_SCALAR_HPP_
