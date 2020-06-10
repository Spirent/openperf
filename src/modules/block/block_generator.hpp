#ifndef _OP_BLOCK_GENERATOR_HPP_
#define _OP_BLOCK_GENERATOR_HPP_

#include "task.hpp"
#include "virtual_device.hpp"
#include "models/generator.hpp"
#include "models/generator_result.hpp"
#include "utils/worker/worker.hpp"

namespace openperf::block::generator {

using namespace openperf::block::worker;

using block_worker = utils::worker::worker<block_task>;
using block_worker_ptr = std::unique_ptr<block_worker>;
using block_result_ptr = std::shared_ptr<model::block_generator_result>;

class block_generator : public model::block_generator
{
private:
    block_worker_ptr m_read_worker, m_write_worker;
    std::string m_statistics_id;
    std::vector<virtual_device_stack*> m_vdev_stack_list;
    std::shared_ptr<virtual_device> m_vdev;
    task_config_t generate_worker_config(const model::block_generator_config&,
                                         task_operation);
    void update_resource(const std::string&);

public:
    ~block_generator();
    block_generator(const model::block_generator& generator_model,
                    const std::vector<virtual_device_stack*>& vdev_stack_list);
    block_result_ptr start();
    void stop();

    void set_config(const model::block_generator_config&);
    void set_resource_id(const std::string&);
    void set_running(bool);

    block_result_ptr get_statistics() const;
    void clear_statistics();
};

} // namespace openperf::block::generator

#endif /* _OP_BLOCK_GENERATOR_HPP_ */
