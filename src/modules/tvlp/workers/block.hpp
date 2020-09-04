#ifndef _OP_TVLP_BLOCK_WORKER_HPP_
#define _OP_TVLP_BLOCK_WORKER_HPP_

#include "tvlp/worker.hpp"

namespace openperf::tvlp::internal::worker {

class block_tvlp_worker_t : public tvlp_worker_t
{
private:
    const std::string m_generator_endpoint = "/block-generators";
    const std::string m_generator_results_endpoint = "/block-generator-results";

public:
    block_tvlp_worker_t() = delete;
    block_tvlp_worker_t(const block_tvlp_worker_t&) = delete;
    block_tvlp_worker_t(const model::tvlp_module_profile_t&);
    ~block_tvlp_worker_t() = default;

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