#ifndef _ICP_MEMORY_SHARED_SEGMENT_H_
#define _ICP_MEMORY_SHARED_SEGMENT_H_

#include <string>

namespace icp {
namespace memory {

class shared_segment
{
    std::string m_path;
    void* m_ptr;
    size_t m_size;
    bool m_initialized;

public:
    shared_segment(const std::string_view path, size_t size, bool create = false);
    shared_segment(const std::string_view path, size_t size, const void* addr);
    ~shared_segment();

    shared_segment& operator=(shared_segment&& other);
    shared_segment(shared_segment&& other);

    shared_segment(const shared_segment&) = delete;
    shared_segment& operator=(const shared_segment&&) = delete;

    std::string_view name() const { return (m_path); }
    size_t size() const { return (m_size); }
    void* get() const { return (m_ptr); }
};

}
}

#endif /* _ICP_MEMORY_SHARED_SEGMENT_H_ */
