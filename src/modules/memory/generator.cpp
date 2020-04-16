#include "memory/generator.hpp"
#include "memory/task_memory.hpp"

#include <cinttypes>
#include <sys/mman.h>

namespace openperf::memory::internal {

// Constructors & Destructor
generator::generator()
    : _stopped(true)
    , _paused(true)
    , _buffer{.ptr = nullptr, .size = 0}
{}

generator::generator(generator&& g)
    : _stopped(g._stopped)
    , _paused(g._paused)
    , _read_workers(std::move(g._read_workers))
    , _write_workers(std::move(g._write_workers))
    , _config(g._config)
    , _buffer{.ptr = nullptr, .size = 0}
{}

generator::generator(const generator::config_t& c)
    : generator()
{
    config(c);
}

generator::~generator()
{
    stop();
    free_buffer();
}

// Methods : public
void generator::start()
{
    if (!_stopped) return;

    for_each_worker([](worker_ptr& w) { w->start(); });
    _stopped = false;
}

void generator::stop()
{
    if (_stopped) return;

    for_each_worker([](worker_ptr& w) { w->stop(); });
    _stopped = true;
}

void generator::restart()
{
    stop();
    start();
}

void generator::resume()
{
    if (!_paused) return;

    for_each_worker([](worker_ptr& w) { w->resume(); });
    _paused = false;
}

void generator::pause()
{
    if (_paused) return;

    for_each_worker([](worker_ptr& w) { w->pause(); });
    _paused = true;
}

void generator::reset()
{
    for_each_worker([](worker_ptr& w) {
        auto wtm = reinterpret_cast<worker<task_memory>*>(w.get());
        wtm->clear_stat();
    });
}

generator::stat_t generator::stat() const
{
    generator::stat_t result_stat;

    auto& rstat = result_stat.read;
    for (auto& ptr : _read_workers) {
        auto w = reinterpret_cast<worker<task_memory>*>(ptr.get());
        rstat += w->stat();
    }

    auto& wstat = result_stat.write;
    for (auto& ptr : _write_workers) {
        auto w = reinterpret_cast<worker<task_memory>*>(ptr.get());
        wstat += w->stat();
    }

    result_stat.timestamp = std::max(rstat.timestamp, wstat.timestamp);
    result_stat.active = is_running();
    return result_stat;
}

generator::config_t generator::config() const { return _config; }

void generator::config(const generator::config_t& cfg)
{
    pause();
    resize_buffer(cfg.buffer_size);
    reallocate_workers<task_memory_read>(_read_workers, cfg.read_threads);
    reallocate_workers<task_memory_write>(_write_workers, cfg.write_threads);

    spread_config(_read_workers,
                  task_memory_config{
                      .block_size = cfg.read.block_size,
                      .op_per_sec = cfg.read.op_per_sec,
                      .pattern = cfg.read.pattern,
                      .buffer = {.ptr = _buffer.ptr, .size = _buffer.size}});
    spread_config(_write_workers,
                  task_memory_config{
                      .block_size = cfg.write.block_size,
                      .op_per_sec = cfg.write.op_per_sec,
                      .pattern = cfg.write.pattern,
                      .buffer = {.ptr = _buffer.ptr, .size = _buffer.size}});

    _config = cfg;
    resume();
}

// Methods : private
void generator::for_each_worker(std::function<void(worker_ptr&)> function)
{
    for (auto list : {&_read_workers, &_write_workers}) {
        for (auto& worker : *list) { function(worker); }
    }
}

void generator::free_buffer()
{
    if (_buffer.ptr == nullptr) return;
    if (_buffer.ptr == MAP_FAILED) return;

    if (munmap(_buffer.ptr, _buffer.size) == -1) {
        OP_LOG(OP_LOG_ERROR,
               "Failed to unallocate %zu"
               " bytes of memory: %s\n",
               _buffer.size,
               strerror(errno));
    }

    _buffer.ptr = MAP_FAILED;
    _buffer.size = 0;
}

void generator::resize_buffer(size_t size)
{
    if (size == _buffer.size) return;

    /*
     * We use mmap/munmap here instead of the standard allocation functions
     * because we expect this buffer to be relatively large, and OSv currently
     * can only allocate non-contiguous chunks of memory with mmap.
     * The buffer contents don't currently matter, so there is no reason to
     * initialize new buffers or shuffle contents between old and new ones.
     */
    free_buffer();
    if (size > 0) {
        OP_LOG(OP_LOG_INFO,
               "Reallocating buffer (%zu => %zu)\n",
               _buffer.size,
               size);
        _buffer.ptr = mmap(NULL,
                           size,
                           PROT_READ | PROT_WRITE,
                           MAP_ANONYMOUS | MAP_PRIVATE,
                           -1,
                           0);
        if (_buffer.ptr == MAP_FAILED) {
            OP_LOG(OP_LOG_ERROR,
                   "Failed to allocate %" PRIu64 " byte memory buffer\n",
                   size);
            return;
        }
        _buffer.size = size;
    }
}

void generator::spread_config(generator::workers& wkrs,
                              const task_memory_config& config)
{
    for (auto& w : wkrs) {
        auto wt = reinterpret_cast<worker<task_memory>*>(w.get());
        wt->config(config);
    }
}

} // namespace openperf::memory::internal
