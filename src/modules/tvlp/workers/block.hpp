#ifndef _OP_TVLP_BLOCK_WORKER_HPP_
#define _OP_TVLP_BLOCK_WORKER_HPP_

#include "tvlp/worker.hpp"
#include "api/api_internal_client.hpp"

namespace openperf::tvlp::internal::worker {

class block_tvlp_worker_t : public tvlp_worker_t
{
public:
    block_tvlp_worker_t() = delete;
    block_tvlp_worker_t(const block_tvlp_worker_t&) = delete;
    block_tvlp_worker_t(const model::tvlp_module_profile_t&);
    ~block_tvlp_worker_t() = default;

protected:
    tl::expected<std::string, std::string>
    send_create(const nlohmann::json&, const std::string&) override;
    tl::expected<stat_pair_t, std::string>
    send_start(const std::string&) override;
    tl::expected<void, std::string> send_stop(const std::string&) override;
    tl::expected<nlohmann::json, std::string>
    send_stat(const std::string&) override;
    tl::expected<void, std::string> send_delete(const std::string&) override;
};

} // namespace openperf::tvlp::internal::worker

#endif