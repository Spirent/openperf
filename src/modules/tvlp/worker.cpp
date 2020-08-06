#include <chrono>
#include "worker.hpp"

namespace openperf::tvlp::internal {

using namespace std::chrono_literals;
tvlp_worker_t::tvlp_worker_t(const model::tvlp_module_profile_t&) {}

void tvlp_worker_t::start(uint64_t delay)
{
    if (scheduler_thread.valid()) scheduler_thread.wait();
    scheduler_thread = std::async(std::launch::async, [this]() { schedule(); });
}

void tvlp_worker_t::stop() {}

void tvlp_worker_t::schedule()
{
    for (auto i = 0; i < 100; i++) {
        std::this_thread::sleep_for(100ms);
        printf("=\n");
    }
}

} // namespace openperf::tvlp::internal