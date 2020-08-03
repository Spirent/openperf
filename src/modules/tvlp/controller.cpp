#include "controller.hpp"

namespace openperf::tvlp::internal {

static uint16_t serial_counter = 0;

controller_t::controller_t(const model::tvlp_configuration_t& model)
    : model::tvlp_configuration_t(model)
    , m_controller(NAME_PREFIX + std::to_string(serial_counter++) + "_ctl")
{}
controller_t::~controller_t() {}

void controller_t::start() {}
void controller_t::start_delayed() {}
void controller_t::stop() {}

} // namespace openperf::tvlp::internal