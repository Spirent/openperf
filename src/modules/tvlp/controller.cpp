#include <algorithm>
#include "controller.hpp"
#include "workers/block.hpp"

namespace openperf::tvlp::internal {

// static uint16_t serial_counter = 0;

controller_t::controller_t(const model::tvlp_configuration_t& model)
    : model::tvlp_configuration_t(model)
{

    auto get_length = [](const model::tvlp_module_profile_t& profiles) {
        uint64_t total_length = 0;
        for (const auto& p : profiles) {
            if (p.length <= 0) throw std::runtime_error("Invalid field value");
            total_length += p.length;
        }
        return total_length;
    };

    uint64_t total_length = 0;
    if (m_profile.block) {
        total_length = get_length(m_profile.block.value());
        m_block = std::make_unique<worker::block_tvlp_worker_t>(
            m_profile.block.value());
    }
    if (m_profile.memory) {
        total_length =
            std::max(total_length, get_length(m_profile.memory.value()));
        m_memory = std::make_unique<worker::block_tvlp_worker_t>(
            m_profile.memory.value());
    }
    if (m_profile.cpu) {
        total_length =
            std::max(total_length, get_length(m_profile.cpu.value()));
        m_cpu = std::make_unique<worker::block_tvlp_worker_t>(
            m_profile.cpu.value());
    }
    if (m_profile.packet) {
        total_length =
            std::max(total_length, get_length(m_profile.packet.value()));
        m_packet = std::make_unique<worker::block_tvlp_worker_t>(
            m_profile.packet.value());
    }

    if (total_length == 0) throw std::runtime_error("Invalid field value");

    m_total_length = total_length;
}
controller_t::~controller_t() {}

void controller_t::start()
{
    if (m_profile.block) {
        m_block->start();
    }
    if (m_profile.memory) {
        m_memory->start();
    }
    if (m_profile.cpu) {
        m_cpu->start();
    }
    if (m_profile.packet) {
        m_packet->start();
    }
}

void controller_t::start_delayed() {}

void controller_t::stop() {}

} // namespace openperf::tvlp::internal