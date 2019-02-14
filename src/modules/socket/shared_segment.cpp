#include <string.h>
#include <stdexcept>

#include <cerrno>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

#include "socket/shared_segment.h"

namespace icp {
namespace memory {

static void* do_mapping(const std::string_view path, size_t size,
                        int shm_flags,
                        const void *addr = nullptr)
{
    auto fd = shm_open(path.data(), shm_flags,
                       S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP);
    if (fd == -1) {
        throw std::runtime_error("Could not "
                                 + (std::string(shm_flags & O_CREAT ? "create" : "attach to"))
                                 + " shared memory segment " + std::string(path) + ": "
                                 + strerror(errno));
    }

    if (shm_flags & O_CREAT) {
        ftruncate(fd, size);
    }

    auto ptr = mmap(const_cast<void*>(addr), size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    close(fd);  /* done with this regardless of outcome... */
    if (ptr == MAP_FAILED) {
        throw std::runtime_error("Could not map shared memory segment "
                                 + std::string(path) + ": " + strerror(errno));
    }

    return (ptr);
}

shared_segment::shared_segment(const std::string_view path, size_t size, bool create, bool unlink_first)
    : m_path(path)
    , m_size(size)
    , m_initialized(false)
{
    auto flags = O_RDWR;
    if (create) {
        flags |= O_CREAT | O_EXCL;
    }
    if (unlink_first) {
        shm_unlink(path.data());
    }
    m_ptr = do_mapping(path, size, flags);
    m_initialized = create;
}

shared_segment::shared_segment(const std::string_view path, size_t size, const void* ptr)
    : m_path(path)
    , m_size(size)
    , m_initialized(false)
{
    m_ptr = do_mapping(path, size, O_RDWR, ptr);
    if (m_ptr != ptr) {
        /* two chars per byte + terminating null */
        static constexpr size_t ptr_buf_length = sizeof(void*) * 2 + 1;
        char buffer[ptr_buf_length];
        std::snprintf(buffer, ptr_buf_length, "%p", ptr);
        throw std::runtime_error("Could not map shared memory segment "
                                 + std::string(path) + " to desired address " + buffer);
    }
}

shared_segment::shared_segment (shared_segment&& other)
    : m_path(std::move(other.m_path))
    , m_ptr(other.m_ptr)
    , m_size(other.m_size)
    , m_initialized(other.m_initialized)
{
    other.m_initialized = false;
}

shared_segment& shared_segment::operator=(shared_segment&& other)
{
    if (this != &other) {
        m_path.swap(other.m_path);
        m_ptr = other.m_ptr;
        m_size = other.m_size;
        m_initialized = other.m_initialized;
        other.m_initialized = false;
    }
    return (*this);
}

shared_segment::~shared_segment()
{
    if (!m_initialized) return;
    munmap(m_ptr, m_size);
    shm_unlink(m_path.data());
}

}
}
