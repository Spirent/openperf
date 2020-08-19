#include <algorithm>
#include "json.hpp"

#include "framework/core/op_core.h"

#include "controller.hpp"
#include "workers/block.hpp"
#include "workers/memory.hpp"
#include "workers/cpu.hpp"
#include "workers/packet.hpp"

namespace openperf::tvlp::internal {

using namespace std::chrono_literals;
// static uint16_t serial_counter = 0;

controller_t::controller_t(const model::tvlp_configuration_t& model)
    : model::tvlp_configuration_t(model)
{

    auto get_length = [](const model::tvlp_module_profile_t& profiles) {
        duration total_length = 0ms;
        for (const auto& p : profiles) {
            if (p.length <= 0ms)
                throw std::runtime_error("Invalid field value");
            total_length += p.length;
        }
        return total_length;
    };

    duration total_length = 0ms;
    if (m_profile.block) {
        total_length = get_length(m_profile.block.value());
        m_block = std::make_unique<worker::block_tvlp_worker_t>(
            m_profile.block.value());
    }
    if (m_profile.memory) {
        total_length =
            std::max(total_length, get_length(m_profile.memory.value()));
        m_memory = std::make_unique<worker::memory_tvlp_worker_t>(
            m_profile.memory.value());
    }
    if (m_profile.cpu) {
        total_length =
            std::max(total_length, get_length(m_profile.cpu.value()));
        m_cpu =
            std::make_unique<worker::cpu_tvlp_worker_t>(m_profile.cpu.value());
    }
    if (m_profile.packet) {
        total_length =
            std::max(total_length, get_length(m_profile.packet.value()));
        m_packet = std::make_unique<worker::packet_tvlp_worker_t>(
            m_profile.packet.value());
    }

    if (total_length == 0ms) throw std::runtime_error("Invalid field value");

    m_total_length = total_length;

    if (model.id().empty()) id(core::to_string(core::uuid::random()));
}
controller_t::~controller_t() {}

std::shared_ptr<model::tvlp_result_t>
controller_t::start(const time_point& start_time)
{
    auto modules_results = model::tvlp_modules_results_t{};

    if (m_profile.block) {
        modules_results.block = model::json_vector();
        m_block->start(start_time);
    }
    if (m_profile.memory) {
        modules_results.memory = model::json_vector();
        m_memory->start(start_time);
    }
    if (m_profile.cpu) {
        modules_results.cpu = model::json_vector();
        m_cpu->start(start_time);
    }
    if (m_profile.packet) {
        modules_results.packet = model::json_vector();
        m_packet->start(start_time);
    }

    m_start_time = start_time;
    if (start_time > realtime::now())
        m_state = model::COUNTDOWN;
    else
        m_state = model::RUNNING;

    m_result = std::make_shared<model::tvlp_result_t>();
    m_result->tvlp_id(id());
    m_result->id(core::to_string(core::uuid::random()));
    m_result->results(modules_results);
    return m_result;
}

void controller_t::stop()
{
    if (m_profile.block) { m_block->stop(); }
    if (m_profile.memory) { m_memory->stop(); }
    if (m_profile.cpu) { m_cpu->stop(); }
    if (m_profile.packet) { m_packet->stop(); }
}

bool controller_t::is_running() const
{
    return m_state == model::COUNTDOWN || m_state == model::RUNNING;
}

void controller_t::update()
{
    auto recv_state = [this](const worker::tvlp_worker_t& worker) {
        switch (worker.state()) {
        case model::READY: {
            break;
        }
        case model::COUNTDOWN: {
            if (m_state == model::READY) m_state = model::COUNTDOWN;
            break;
        }
        case model::RUNNING: {
            if (m_state != model::ERROR) m_state = model::RUNNING;

            break;
        }
        case model::ERROR: {
            m_error += worker.error().value() + ";";
            m_state = model::ERROR;
            break;
        }
        }
        m_current_offset = std::max(m_current_offset, worker.offset());
        return worker.results();
    };

    m_error = "";
    m_current_offset = duration::zero();
    m_state = model::READY;
    model::tvlp_modules_results_t modules_results;
    if (m_profile.block) { modules_results.block = recv_state(*m_block); }
    if (m_profile.memory) { modules_results.memory = recv_state(*m_memory); }
    if (m_profile.cpu) { modules_results.cpu = recv_state(*m_cpu); }
    if (m_profile.packet) { modules_results.packet = recv_state(*m_packet); }
    if (m_result) m_result->results(modules_results);
}

model::tvlp_configuration_t controller_t::model()
{
    update();
    return model::tvlp_configuration_t(*this);
}

} // namespace openperf::tvlp::internal