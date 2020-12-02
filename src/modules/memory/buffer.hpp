#ifndef _OP_MEMORY_BUFFER_HPP_
#define _OP_MEMORY_BUFFER_HPP_

#include <optional>

namespace openperf::memory::internal {

class buffer
{
private:
    using size_t = std::size_t;

    std::optional<std::size_t> m_alignment;
    void* m_ptr = nullptr;
    std::size_t m_size = 0;

public:
    buffer() = default;
    explicit buffer(size_t alignment);
    buffer(size_t size, size_t alignment);
    buffer(const buffer&);
    buffer(buffer&&) noexcept;
    ~buffer();

    buffer& operator=(const buffer&);

    void allocate(size_t size);
    void resize(size_t size);
    void release();

    bool is_allocated() const { return m_size == 0; }
    size_t size() const { return m_size; }
    void* data() const { return m_ptr; }

private:
    void allocate(size_t, size_t);
    void resize(size_t, size_t);
};

} // namespace openperf::memory::internal

#endif // _OP_MEMORY_BUFFER_HPP_
