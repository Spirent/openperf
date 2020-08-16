#ifndef _OP_TVLP_CONTROLLER_HPP_
#define _OP_TVLP_CONTROLLER_HPP_

#include <future>
#include <atomic>

#include "models/tvlp_config.hpp"
#include "models/tvlp_result.hpp"
#include "framework/generator/controller.hpp"
#include "worker.hpp"

namespace openperf::tvlp::internal {

using framework_controller = openperf::framework::generator::controller;
using duration = std::chrono::nanoseconds;
using time_point = std::chrono::time_point<timesync::chrono::realtime>;
using realtime = timesync::chrono::realtime;

class controller_t : public model::tvlp_configuration_t
{
private:
    static constexpr auto NAME_PREFIX = "op_tvlp";
    std::shared_ptr<model::tvlp_result_t> m_result;
    std::unique_ptr<worker::tvlp_worker_t> m_block, m_memory, m_cpu, m_packet;

public:
    // Constructors & Destructor
    controller_t() = delete;
    explicit controller_t(const model::tvlp_configuration_t&);
    controller_t(const controller_t&) = delete;
    ~controller_t();

    // Operators overloading
    controller_t& operator=(const controller_t&) = delete;

    std::shared_ptr<model::tvlp_result_t> start(const time_point& start_time);
    void stop();

    model::tvlp_configuration_t model();
};

} // namespace openperf::tvlp::internal

#endif // _OP_MEMORY_GENERATOR_HPP_
