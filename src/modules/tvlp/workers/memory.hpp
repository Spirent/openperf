#ifndef _OP_TVLP_MEMORY_WORKER_HPP_
#define _OP_TVLP_MEMORY_WORKER_HPP_

#include "tvlp/worker.hpp"

namespace openperf::tvlp::internal::worker {

class memory_tvlp_worker_t : public tvlp_worker_t
{
private:
    const std::string m_generator_endpoint = "/memory-generators";
    const std::string m_generator_results_endpoint =
        "/memory-generator-results";

public:
    memory_tvlp_worker_t() = delete;
    memory_tvlp_worker_t(const memory_tvlp_worker_t&) = delete;
    memory_tvlp_worker_t(void* context, const model::tvlp_module_profile_t&);
    ~memory_tvlp_worker_t() = default;

protected:
    tl::expected<std::string, std::string>
    send_create(const nlohmann::json&, const std::string&) override;
    tl::expected<stat_pair_t, std::string>
    send_start(const std::string& id) override;
    tl::expected<void, std::string> send_stop(const std::string& id) override;
    tl::expected<nlohmann::json, std::string>
    send_stat(const std::string& id) override;
    tl::expected<void, std::string> send_delete(const std::string& id) override;

    std::string generator_endpoint() override { return m_generator_endpoint; };
    std::string generator_results_endpoint() override
    {
        return m_generator_results_endpoint;
    };
};

} // namespace openperf::tvlp::internal::worker

#endif
