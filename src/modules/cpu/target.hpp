#ifndef _OP_CPU_TARGET_HPP_
#define _OP_CPU_TARGET_HPP_

#include <cinttypes>

class target
{
public:
    enum data_size_t {
        BIT_32 = 32,
        BIT_64 = 64
    };

    enum operation_t {
        FLOAT, INT
    };

protected:
    data_size_t m_data_size;
    operation_t m_operation;
    uint64_t    m_weight;

public:
    target(data_size_t size, operation_t op, uint64_t weight)
        : m_data_size(size), m_operation(op), m_weight(weight)
        {}
    virtual ~target() = default;
    virtual uint64_t operation() const = 0;

    inline uint64_t operator()() const { return operation(); }
};

#endif // _OP_CPU_TARGET_HPP_
