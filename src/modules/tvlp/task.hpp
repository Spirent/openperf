#ifndef _OP_TVLP_TASK_HPP_
#define _OP_TVLP_TASK_HPP_

#include <vector>
#include <chrono>

#include "framework/generator/task.hpp"
#include "modules/timesync/chrono.hpp"

#include "io_pattern.hpp"

namespace openperf::tvlp::internal {

struct task_tvlp_config
{};

struct task_tvlp_stat
{};

class task_tvlp : public openperf::framework::generator::task<task_tvlp_stat>
{
    using chronometer = openperf::timesync::chrono::monotime;

protected:
    task_tvlp_config m_config;
    uint8_t* m_buffer = nullptr;

    struct
    {
        void* ptr;
        size_t size;
    } m_scratch;

    task_tvlp_stat m_stat;
    size_t m_op_index;
    double m_avg_rate;

public:
    task_tvlp() = delete;
    task_tvlp(task_tvlp&&) noexcept;
    task_tvlp(const task_tvlp&) = delete;
    explicit task_tvlp(const task_tvlp_config&);
    ~task_tvlp() override;

    void reset() override;

    task_tvlp_config config() const { return m_config; }
    task_tvlp_stat spin() override;

protected:
    void config(const task_tvlp_config&);
    virtual void operation(uint64_t nb_ops) = 0;
    virtual task_tvlp_stat make_stat(const task_tvlp_stat&) = 0;
};

} // namespace openperf::tvlp::internal

#endif // _OP_TVLP_TASK_HPP_
