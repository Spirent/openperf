#include <cassert>
#include <future>
#include <stdexcept>
#include <string>

#include "core/op_cpuset.hpp"
#include "core/op_thread.h"

namespace openperf::core {

cpuset::cpuset()
    : m_cpuset(op_cpuset_create())
{}

cpuset::cpuset(const char* s)
    : m_cpuset(op_cpuset_create_from_string(s))
{
    if (!m_cpuset) {
        throw std::invalid_argument("Could not generate cpuset from "
                                    + std::string(s));
    }
}

cpuset::cpuset(const std::string& s)
    : m_cpuset(op_cpuset_create_from_string(s.c_str()))
{
    if (!m_cpuset) {
        throw std::invalid_argument("Could not generate cpuset from " + s);
    }
}

cpuset::cpuset(const cpuset& other)
    : m_cpuset(op_cpuset_create())
{
    for (auto i = 0UL; i < other.size(); i++) {
        if (other.test(i)) { set(i); }
    }
}

cpuset& cpuset::operator=(const cpuset& other)
{
    reset();
    for (auto i = 0UL; i < other.size(); i++) {
        if (other.test(i)) { set(i); }
    }
    return (*this);
}

void* cpuset::data() { return (m_cpuset.get()); }

void* cpuset::data() const { return (m_cpuset.get()); }

size_t cpuset::count() const noexcept
{
    return (op_cpuset_count(m_cpuset.get()));
}

size_t cpuset::size() const noexcept
{
    return (op_cpuset_size(m_cpuset.get()));
}

bool cpuset::test(size_t idx) const
{
    if (idx >= size()) {
        throw std::out_of_range("Can't test bit " + std::to_string(idx) + " in "
                                + std::to_string(size()) + " bit set");
    }

    return (op_cpuset_get(m_cpuset.get(), idx));
}

bool cpuset::all() const noexcept { return (count() == size()); }

bool cpuset::any() const noexcept { return (count() > 0); }

bool cpuset::none() const noexcept { return (count() == 0); }

cpuset& cpuset::set() noexcept
{
    op_cpuset_set_range(m_cpuset.get(), 0, size(), true);
    return (*this);
}

cpuset& cpuset::set(size_t idx, bool value)
{
    if (idx >= size()) {
        throw std::out_of_range("Can't set bit " + std::to_string(idx) + " in "
                                + std::to_string(size()) + " bit set");
    }

    op_cpuset_set(m_cpuset.get(), idx, value);
    return (*this);
}

cpuset& cpuset::reset() noexcept
{
    op_cpuset_set_range(m_cpuset.get(), 0, size(), false);
    return (*this);
}

cpuset& cpuset::reset(size_t idx)
{
    if (idx >= size()) {
        throw std::out_of_range("Can't reset bit " + std::to_string(idx)
                                + " in " + std::to_string(size()) + " bit set");
    }

    op_cpuset_set(m_cpuset.get(), idx, false);
    return (*this);
}

cpuset& cpuset::flip() noexcept
{
    for (auto i = 0UL; i < size(); i++) {
        op_cpuset_set(m_cpuset.get(), i, !op_cpuset_get(m_cpuset.get(), i));
    }
    return (*this);
}

std::string cpuset::to_string() const
{
    /*
     * Figure out the maximum length of a hex string for our size.
     * Obviously we have a character for every 16 bits. Additionally,
     * add 3 to cover the prefix, 0x, and the trailing newline.
     */
    auto length = (size() + 15 / 16) + 3;
    auto value = std::string(length, '\0');
    op_cpuset_to_string(m_cpuset.get(), value.data(), value.capacity());

    /* trim string */
    value.resize(strnlen(value.data(), value.capacity()));

    return (value);
}

bool cpuset::operator[](size_t idx) const { return (test(idx)); }

bool cpuset::operator==(const cpuset& other) const noexcept
{
    return (op_cpuset_equal(m_cpuset.get(), other.m_cpuset.get()));
}

bool cpuset::operator!=(const cpuset& other) const noexcept
{
    return (!(*this == other));
}

cpuset& cpuset::operator&=(const cpuset& other) noexcept
{
    op_cpuset_and(m_cpuset.get(), other.m_cpuset.get());
    return (*this);
}

cpuset& cpuset::operator|=(const cpuset& other) noexcept
{
    op_cpuset_or(m_cpuset.get(), other.m_cpuset.get());
    return (*this);
}

cpuset cpuset::operator~() const noexcept
{
    auto value = cpuset(*this);
    value.flip();
    return (value);
}

cpuset operator&(const cpuset& lhs, const cpuset& rhs) noexcept
{
    return (cpuset(lhs) &= rhs);
}

cpuset operator|(const cpuset& lhs, const cpuset& rhs) noexcept
{
    return (cpuset(lhs) |= rhs);
}

int cpuset_set_affinity(const cpuset& set)
{
    return (op_thread_set_affinity_mask(set.data()));
}

cpuset cpuset_get_affinity()
{
    auto set = cpuset();
    op_thread_get_affinity_mask(set.data());
    return (set);
}

cpuset cpuset_online()
{
    auto original = cpuset_get_affinity();

    auto all = cpuset();
    all.set();
    if (cpuset_set_affinity(all)) {
        throw std::runtime_error("Failed to set affinity");
    }

    auto updated = cpuset_get_affinity();

    if (cpuset_set_affinity(original)) {
        throw std::runtime_error("Failed to restore original affinity");
    }

    return (updated);
}

} // namespace openperf::core
