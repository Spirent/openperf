#ifndef _OP_CPUSET_HPP_
#define _OP_CPUSET_HPP_

#include <memory>
#include <string>
#include <type_traits>

#include "core/op_cpuset.h"

namespace openperf::core {

/*
 * Provide a convenient RAII wrapper for working with cpusets
 * that has the same interface as std::bitset.
 */
class cpuset
{
    struct op_cpuset_deleter
    {
        void operator()(void* s) const { op_cpuset_delete(s); }
    };

    std::unique_ptr<std::remove_pointer_t<op_cpuset_t>, op_cpuset_deleter>
        m_cpuset;

public:
    cpuset();
    cpuset(const char*);
    cpuset(const std::string&);

    ~cpuset() = default;

    cpuset(const cpuset& other);
    cpuset& operator=(const cpuset& other);

    void* data();
    void* data() const;

    size_t count() const noexcept;
    size_t size() const noexcept;

    bool test(size_t idx) const;

    bool all() const noexcept;
    bool any() const noexcept;
    bool none() const noexcept;

    cpuset& set() noexcept;
    cpuset& set(size_t idx, bool value = true);

    cpuset& reset() noexcept;
    cpuset& reset(size_t idx);

    cpuset& flip() noexcept;

    std::string to_string() const;

    bool operator[](std::size_t idx) const;
    bool operator==(const cpuset& other) const noexcept;
    bool operator!=(const cpuset& other) const noexcept;
    cpuset& operator&=(const cpuset& other) noexcept;
    cpuset& operator|=(const cpuset& other) noexcept;
    cpuset operator~() const noexcept;

    friend cpuset operator&(const cpuset& lhs, const cpuset& rhs) noexcept;
    friend cpuset operator|(const cpuset& lhs, const cpuset& rhs) noexcept;
};

int cpuset_set_affinity(const cpuset& set);
cpuset cpuset_get_affinity();
cpuset cpuset_online();

} // namespace openperf::core

#endif /* _OP_CPUSET_HPP_ */
