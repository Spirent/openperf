#include <algorithm>
#include "controller.hpp"

namespace openperf::tvlp::internal {

static uint16_t serial_counter = 0;

controller_t::controller_t(const model::tvlp_configuration_t& model)
    : model::tvlp_configuration_t(model)
    , m_controller(NAME_PREFIX + std::to_string(serial_counter++) + "_ctl")
{

    auto get_length =
        [](const std::vector<model::tvlp_profile_entry_t>& profiles) {
            uint64_t total_length = 0;
            for (const auto& p : profiles) {
                if (p.length <= 0)
                    throw std::runtime_error("Invalid field value");
                total_length += p.length;
            }
            return total_length;
        };

    uint64_t total_length = 0;
    if (m_profile.block) total_length = get_length(m_profile.block.value());
    if (m_profile.memory)
        total_length =
            std::max(total_length, get_length(m_profile.memory.value()));
    if (m_profile.cpu)
        total_length =
            std::max(total_length, get_length(m_profile.cpu.value()));
    if (m_profile.packet)
        total_length =
            std::max(total_length, get_length(m_profile.packet.value()));

    if (total_length == 0) throw std::runtime_error("Invalid field value");

    m_total_length = total_length;
}
controller_t::~controller_t() {}

void controller_t::start() {}
void controller_t::start_delayed() {}
void controller_t::stop() {}

} // namespace openperf::tvlp::internal