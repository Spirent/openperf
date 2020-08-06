#include "tvlp/worker.hpp"

namespace openperf::tvlp::internal::worker {

class block_tvlp_worker_t : public tvlp_worker_t
{
public:
    block_tvlp_worker_t() = delete;
    block_tvlp_worker_t(const block_tvlp_worker_t&) = delete;
    block_tvlp_worker_t(const model::tvlp_module_profile_t& profile)
        : tvlp_worker_t(profile){};
    ~block_tvlp_worker_t() = default;

protected:
    inline void send_create() override { printf("send_create \n"); }
    inline void send_start() override { printf("send_start \n"); }
    inline void send_stop() override { printf("send_stop \n"); }
    inline void send_stat() override { printf("send_stat \n"); }
    inline void send_delete() override { printf("send_delete \n"); }
};

} // namespace openperf::tvlp::internal::worker