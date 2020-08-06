#ifndef _OP_TVLP_WORKER_HPP_
#define _OP_TVLP_WORKER_HPP_

#include <vector>
#include <chrono>
#include <future>

#include "framework/generator/task.hpp"
#include "modules/timesync/chrono.hpp"
#include "models/tvlp_config.hpp"

namespace openperf::tvlp::internal {

struct tvlp_worker_state_t
{};

class tvlp_worker_t
{
public:
    tvlp_worker_t() = delete;
    tvlp_worker_t(const tvlp_worker_t&) = delete;
    tvlp_worker_t(const model::tvlp_module_profile_t&);
    virtual ~tvlp_worker_t() = default;

    void start(uint64_t delay = 0);
    void stop();

protected:
    void schedule();
    virtual void send_create() = 0;
    virtual void send_start() = 0;
    virtual void send_stop() = 0;
    virtual void send_stat() = 0;
    virtual void send_delete() = 0;

    tvlp_worker_state_t m_state;
    std::future<void> scheduler_thread;
};

} // namespace openperf::tvlp::internal

#endif