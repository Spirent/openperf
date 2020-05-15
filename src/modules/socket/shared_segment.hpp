#ifndef _OP_MEMORY_SHARED_SEGMENT_HPP_
#define _OP_MEMORY_SHARED_SEGMENT_HPP_

#include <string>

namespace openperf::memory {

class shared_segment
{
    std::string m_path;
    void* m_ptr;
    size_t m_size;
    bool m_initialized;

public:
    shared_segment(const std::string_view path,
                   size_t size,
                   bool create = false);
    ~shared_segment();

    shared_segment& operator=(shared_segment&& other) noexcept;
    shared_segment(shared_segment&& other) noexcept;

    shared_segment(const shared_segment&) = delete;
    shared_segment& operator=(const shared_segment&&) = delete;

    std::string_view name() const { return (m_path); }
    size_t size() const { return (m_size); }
    void* base() const { return (m_ptr); }
};

} // namespace openperf::memory

#endif /* _OP_MEMORY_SHARED_SEGMENT_HPP_ */
