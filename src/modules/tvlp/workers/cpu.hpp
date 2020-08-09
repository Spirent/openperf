#include "tvlp/worker.hpp"

namespace openperf::tvlp::internal::worker {

class cpu_tvlp_worker_t : public tvlp_worker_t
{
public:
    cpu_tvlp_worker_t() = delete;
    cpu_tvlp_worker_t(const cpu_tvlp_worker_t&) = delete;
    cpu_tvlp_worker_t(const model::tvlp_module_profile_t& profile)
        : tvlp_worker_t(profile){};
    ~cpu_tvlp_worker_t() = default;

protected:
    inline tl::expected<std::string, std::string>
    send_create(const nlohmann::json&, const std::string&) override
    {
        printf("send_create \n");
        return "";
    }
    inline tl::expected<std::string, std::string>
    send_start(const std::string&) override
    {
        printf("send_start \n");
        return "";
    }
    inline tl::expected<void, std::string>
    send_stop(const std::string&) override
    {
        printf("send_stop \n");
        return {};
    }
    inline tl::expected<std::string, std::string>
    send_stat(const std::string&) override
    {
        printf("send_stat \n");
        return "";
    }
    inline tl::expected<void, std::string>
    send_delete(const std::string&) override
    {
        printf("send_delete \n");
        return {};
    }
};

} // namespace openperf::tvlp::internal::worker