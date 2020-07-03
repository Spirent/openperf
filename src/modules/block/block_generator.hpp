#ifndef _OP_BLOCK_GENERATOR_HPP_
#define _OP_BLOCK_GENERATOR_HPP_

#include "task.hpp"
#include "virtual_device.hpp"
#include "models/generator.hpp"
#include "models/generator_result.hpp"

#include "framework/generator/controller.hpp"
#include "framework/dynamic/spool.hpp"

#include "modules/timesync/chrono.hpp"

namespace openperf::block::generator {

using namespace openperf::block::worker;

class block_generator : public model::block_generator
{
    using block_result_ptr = std::shared_ptr<model::block_generator_result>;
    using chronometer = openperf::timesync::chrono::realtime;

private:
    static constexpr auto NAME_PREFIX = "op_block";

private:
    uint16_t m_serial_number;
    framework::generator::controller m_controller;

    task_synchronizer m_synchronizer;
    std::string m_statistics_id;
    std::vector<virtual_device_stack*> m_vdev_stack_list;
    std::shared_ptr<virtual_device> m_vdev;

    task_stat_t m_read, m_write;
    chronometer::time_point m_start_time;

    dynamic::spool<model::block_generator_result> m_dynamic;

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
};

} // namespace openperf::block::generator

#endif /* _OP_BLOCK_GENERATOR_HPP_ */
