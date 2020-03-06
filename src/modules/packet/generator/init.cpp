#include "core/op_core.h"

namespace openperf::packet::generator {

static constexpr int module_version = 1;

}

REGISTER_MODULE(packet_generator,
                INIT_MODULE_INFO("packet-generator",
                                 "Module that generates network packet traffic",
                                 openperf::packet::generator::module_version),
                nullptr,
                nullptr,
                nullptr,
                nullptr,
                nullptr,
                nullptr);
