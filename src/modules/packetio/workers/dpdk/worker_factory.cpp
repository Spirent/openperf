#include "core/op_core.h"
#include "packetio/generic_workers.hpp"
#include "packetio/workers/dpdk/worker_controller.hpp"

namespace openperf::packetio::workers {

std::unique_ptr<generic_workers> make(void* context,
                                      openperf::core::event_loop& loop,
                                      driver::generic_driver& driver)
{
    return (std::make_unique<generic_workers>(
        dpdk::worker_controller(context, loop, driver)));
}

} // namespace openperf::packetio::workers
