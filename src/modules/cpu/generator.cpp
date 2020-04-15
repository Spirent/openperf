#include <cstring>
#include <stdexcept>
#include "generator.hpp"
#include "core/op_uuid.hpp"

namespace openperf::cpu::generator {

cpu_generator::cpu_generator(const model::cpu_generator& generator_model)
    : model::cpu_generator(generator_model)
{

}

cpu_generator::~cpu_generator()
{}

cpu_result_ptr cpu_generator::start() {
    set_running(true);
    return get_statistics();
}

void cpu_generator::stop() { set_running(false); }

void cpu_generator::set_config(const model::cpu_generator_config& value)
{
    model::cpu_generator::set_config(value);
}

void cpu_generator::set_running(bool value)
{
    model::cpu_generator::set_running(value);
}

cpu_result_ptr cpu_generator::get_statistics() const
{
    auto gen_stat = std::make_shared<model::cpu_generator_result>();
    gen_stat->set_id(core::to_string(core::uuid::random()));
    gen_stat->set_generator_id(get_id());
    gen_stat->set_active(is_running());
    gen_stat->set_timestamp(model::time_point::clock::now());
    gen_stat->set_stats({});
    return gen_stat;
}

void cpu_generator::clear_statistics() { }


} // namespace openperf::cpu::generator