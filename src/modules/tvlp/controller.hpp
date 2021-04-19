#ifndef _OP_TVLP_CONTROLLER_HPP_
#define _OP_TVLP_CONTROLLER_HPP_

#include <atomic>
#include <memory>

#include "tvlp/models/tvlp_config.hpp"

namespace openperf::tvlp {

namespace model {
class tvlp_result_t;
}

namespace internal {

namespace worker {
class tvlp_worker_t;
}

class controller_t : public model::tvlp_configuration_t
{
    static constexpr auto NAME_PREFIX = "op_tvlp";

private:
    void* m_context;
    std::shared_ptr<model::tvlp_result_t> m_result;
    std::unique_ptr<worker::tvlp_worker_t> m_block, m_memory, m_cpu, m_packet,
        m_network;

public:
    // Constructors & Destructor
    controller_t() = delete;
    explicit controller_t(void* context, const model::tvlp_configuration_t&);
    controller_t(const controller_t&) = delete;
    ~controller_t() = default;

    // Operators overloading
    controller_t& operator=(const controller_t&) = delete;

    std::shared_ptr<model::tvlp_result_t>
    start(const model::tvlp_start_t& start_configuration);
    void stop();
    bool is_running() const;

    void update();
    model::tvlp_configuration_t model();
};

} // namespace internal
} // namespace openperf::tvlp

#endif // _OP_MEMORY_GENERATOR_HPP_
