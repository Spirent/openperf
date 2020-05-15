#include <string.h>
#include <stdexcept>

#include <cerrno>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

#include "socket/shared_segment.hpp"

namespace openperf {
namespace memory {

static void* do_mapping(const std::string_view path, size_t size, int shm_flags)
{
    auto fd =
        shm_open(path.data(), shm_flags, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP);
    if (fd == -1) {
        if (errno == EEXIST && shm_flags & O_CREAT) {
            throw std::runtime_error(
                "Could not create shared memory segment \"" + std::string(path)
                + "\". Either OpenPerf is already "
                + "running or did not shut down cleanly. "
                + "Use --modules.socket.force-unlink option to force launch.");
        } else {
            throw std::runtime_error(
                "Could not "
                + (std::string(shm_flags & O_CREAT ? "create" : "attach to"))
                + " shared memory segment " + std::string(path) + ": "
                + strerror(errno));
        }
    }

    if (shm_flags & O_CREAT) { ftruncate(fd, size); }

    auto ptr = mmap(nullptr, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    close(fd); /* done with this regardless of outcome... */
    if (ptr == MAP_FAILED) {
        throw std::runtime_error("Could not map shared memory segment "
                                 + std::string(path) + ": " + strerror(errno));
    }

    return (ptr);
}

shared_segment::shared_segment(const std::string_view path,
                               size_t size,
                               bool create)
    : m_path(path)
    , m_size(size)
    , m_initialized(false)
{
    auto flags = O_RDWR;
    if (create) { flags |= O_CREAT | O_EXCL; }
    m_ptr = do_mapping(path, size, flags);
    m_initialized = create;
}

shared_segment::shared_segment(shared_segment&& other) noexcept
    : m_path(std::move(other.m_path))
    , m_ptr(other.m_ptr)
    , m_size(other.m_size)
    , m_initialized(other.m_initialized)
{
    other.m_initialized = false;
}

shared_segment& shared_segment::operator=(shared_segment&& other) noexcept
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

} // namespace memory
} // namespace openperf
