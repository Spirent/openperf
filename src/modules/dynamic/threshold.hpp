#ifndef _OP_DYNAMIC_THRESHOLD_HPP_
#define _OP_DYNAMIC_THRESHOLD_HPP_

#include <cinttypes>

namespace openperf::dynamic {

enum class comparator {
    EQUAL = 1,
    GREATER_THAN,
    GREATER_OR_EQUAL,
    LESS_THAN,
    LESS_OR_EQUAL
};

class threshold
{
private:
    comparator m_comparator = comparator::EQUAL;
    double m_value = 0.0;
    uint64_t m_true = 0;
    uint64_t m_false = 0;

public:
    threshold(double value, comparator cmp)
        : m_comparator(cmp)
        , m_value(value)
    {}

    threshold(const threshold& t)
        : m_comparator(t.m_comparator)
        , m_value(t.m_value)
        , m_true(t.m_true)
        , m_false(t.m_false)
    {}

    template <typename T> void append(T value)
    {
        switch (m_comparator) {
        case comparator::EQUAL:
            inc_counter(value == m_value);
            break;
        case comparator::GREATER_THAN:
            inc_counter(value > m_value);
            break;
        case comparator::GREATER_OR_EQUAL:
            inc_counter(value >= m_value);
            break;
        case comparator::LESS_THAN:
            inc_counter(value < m_value);
            break;
        case comparator::LESS_OR_EQUAL:
            inc_counter(value <= m_value);
            break;
        }
    };

    void reset() { m_true = m_false = 0; }
    comparator condition() const { return m_comparator; }
    double value() const { return m_value; }
    uint64_t trues() const { return m_true; }
    uint64_t falses() const { return m_false; }
    uint64_t count() const { return m_true + m_false; }

private:
    void inc_counter(bool is_true) { (is_true) ? ++m_true : ++m_false; }
};

} // namespace openperf::dynamic

#endif // _OP_DYNAMIC_THRESHOLD_HPP_
