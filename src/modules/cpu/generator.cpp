#include "generator.hpp"

#include "framework/core/op_uuid.hpp"
#include "cpu.hpp"

namespace openperf::cpu::generator {

static uint16_t serial_counter = 0;
constexpr auto NAME_PREFIX = "op_cpu";

// Constructors & Destructor
generator::generator(const model::generator& generator_model)
    : model::generator(generator_model)
    , m_result_id(core::to_string(core::uuid::random()))
    , m_serial_number(++serial_counter)
    , m_controller(NAME_PREFIX + std::to_string(m_serial_number) + "_ctl")
    , m_stat_ptr(&m_stat)
{
    config(generator_model.config());
    m_controller.start<task_cpu_stat*>([this](const task_cpu_stat& stat) {
        auto stat_copy = m_stat;
        m_stat_ptr = &stat_copy;
        m_stat += stat;

        if (m_stat.steal == 0ns) m_stat.steal = cpu_steal_time();

        m_stat_ptr = &m_stat;
    });
}

generator::~generator()
{
    stop();
    m_controller.clear();
}

// Methods : public
void generator::config(const model::generator_config& config)
{
    auto was_running = running();
    m_controller.pause();

    auto cores_count = cpu_cores();
    if (static_cast<int32_t>(config.cores.size()) > cores_count)
        throw std::runtime_error(
            "Could not configure more cores than available ("
            + std::to_string(cores_count) + ").");

    m_stat = {config.cores.size()};

    for (size_t core = 0; core < config.cores.size(); ++core) {
        auto core_conf = config.cores.at(core);
        for (const auto& target : core_conf.targets)
            if (!is_supported(target.set))
                throw std::runtime_error("Instruction set "
                                         + std::string(to_string(target.set))
                                         + " is not supported");

        if (core_conf.utilization == 0.0) continue;

        core_conf.core = core;
        auto task = task_cpu(core_conf);
        m_controller.add(std::move(task),
                         NAME_PREFIX + std::to_string(m_serial_number) + "_c"
                             + std::to_string(core + 1),
                         core);
    }

    model::generator::config(config);

    if (was_running) m_controller.resume();
}

void generator::start()
{
    reset();
    m_controller.resume();
    model::generator::running(true);
}

void generator::stop()
{
    m_controller.pause();
    model::generator::running(false);
}

void generator::running(bool is_running)
{
    if (is_running)
        start();
    else
        stop();
}

model::generator_result generator::statistics() const
{
    auto stat = model::generator_result{};
    stat.id(m_result_id);
    stat.generator_id(id());
    stat.active(running());
    stat.timestamp(timesync::chrono::realtime::now());
    stat.stats(*m_stat_ptr);

    return stat;
}

void generator::reset()
{
    auto was_running = running();
    m_controller.pause();
    m_controller.reset();
    m_stat = {m_stat.cores.size()};
    m_result_id = core::to_string(core::uuid::random());

    if (was_running) m_controller.resume();
}

// Methods : private
bool generator::is_supported(cpu::instruction_set iset)
{
    switch (iset) {
    case cpu::instruction_set::SCALAR:
        return true;
    default:
        return false;
    }
}

} // namespace openperf::cpu::generator
