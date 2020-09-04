#ifndef _OP_TVLP_PACKET_WORKER_HPP_
#define _OP_TVLP_PACKET_WORKER_HPP_

#include <string_view>
#include "tvlp/worker.hpp"

namespace openperf::tvlp::internal::worker {

class packet_tvlp_worker_t : public tvlp_worker_t
{
private:
    const std::string m_generator_endpoint = "/packet/generators";
    const std::string m_generator_results_endpoint =
        "/packet/generator-results";

public:
    packet_tvlp_worker_t() = delete;
    packet_tvlp_worker_t(const packet_tvlp_worker_t&) = delete;
    packet_tvlp_worker_t(const model::tvlp_module_profile_t&);
    ~packet_tvlp_worker_t() = default;

protected:
    tl::expected<std::string, std::string>
    send_create(const nlohmann::json&, const std::string&) override;

    std::string generator_endpoint() override { return m_generator_endpoint; };
    std::string generator_results_endpoint() override
    {
        return m_generator_results_endpoint;
    };
};

} // namespace openperf::tvlp::internal::worker

#endif
