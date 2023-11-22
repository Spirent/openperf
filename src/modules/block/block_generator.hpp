#ifndef _OP_BLOCK_GENERATOR_HPP_
#define _OP_BLOCK_GENERATOR_HPP_

#include "block/task.hpp"
#include "framework/generator/controller.hpp"
#include "dynamic/spool.hpp"
#include "timesync/chrono.hpp"

namespace openperf::block {

class virtual_device;
class virtual_device_stack;

namespace model {
class block_generator;
class block_generator_result;
} // namespace model

namespace generator {

using namespace openperf::block::worker;

class block_generator : public model::block_generator
{
    using block_result_ptr = std::shared_ptr<model::block_generator_result>;
    using chronometer = openperf::timesync::chrono::realtime;
    using time_point = timesync::chrono::realtime::time_point;

public:
    struct block_stat
    {
        task_stat_t read = {.operation = task_operation::READ};
        task_stat_t write = {.operation = task_operation::WRITE};
    };

private:
    static constexpr auto NAME_PREFIX = "op_block";

private:
    uint16_t m_serial_number;

    task_synchronizer m_synchronizer;
    std::string m_statistics_id;
    std::vector<virtual_device_stack*> m_vdev_stack_list;
    std::shared_ptr<virtual_device> m_vdev;

    block_stat m_stat;
    std::atomic<block_stat*> m_stat_ptr;

    chronometer::time_point m_start_time;

    dynamic::spool<block_stat> m_dynamic;
    framework::generator::controller m_controller;

    uint32_t m_op_mask = 0;
    uint32_t m_dynamic_op_mask = 0;

public:
    block_generator(const model::block_generator& generator_model,
                    const std::vector<virtual_device_stack*>& vdev_stack_list);
    ~block_generator() override;

    void reset();
    block_result_ptr start();
    block_result_ptr start(const dynamic::configuration&);
    void stop();

    void config(const model::block_generator_config&) override;
    void resource_id(std::string_view) override;
    void running(bool) override;

    block_result_ptr statistics() const;

private:
    void update_resource(const std::string&);
    task_config_t make_task_config(const model::block_generator_config&,
                                   task_operation);
    void set_task_stopping(bool stopping);
};

} // namespace generator
} // namespace openperf::block

#endif /* _OP_BLOCK_GENERATOR_HPP_ */
