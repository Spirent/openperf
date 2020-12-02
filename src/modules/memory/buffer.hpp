#ifndef _OP_MEMORY_BUFFER_HPP_
#define _OP_MEMORY_BUFFER_HPP_

#include <cinttypes>
#include <optional>

namespace openperf::memory::internal {

class buffer
{
public:
    enum mechanism : uint8_t {
        STDLIB = 0,
        MMAP,
    };

private:
    using size_t = std::size_t;

    mechanism m_mode;
    std::optional<std::size_t> m_alignment;
    void* m_ptr = nullptr;
    std::size_t m_size = 0;

public:
    explicit buffer(mechanism = STDLIB);
    explicit buffer(size_t alignment, mechanism = STDLIB);
    buffer(size_t size, size_t alignment, mechanism = STDLIB);
    buffer(const buffer&);
    buffer(buffer&&) noexcept;
    ~buffer();

    buffer& operator=(const buffer&);

    bool allocate(size_t size);
    bool resize(size_t size);
    void release();

    bool is_allocated() const { return m_size == 0; }
    size_t size() const { return m_size; }
    void* data() const { return m_ptr; }

private:
    bool allocate(size_t, size_t);
    bool resize(size_t, size_t);
};

} // namespace openperf::memory::internal

#endif // _OP_MEMORY_BUFFER_HPP_
