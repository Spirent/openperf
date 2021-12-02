#ifndef _OP_NETWORK_ARG_PARSER_HPP_
#define _OP_NETWORK_ARG_PARSER_HPP_

#include <chrono>
#include <memory>
#include <optional>

#include "core/op_cpuset.hpp"
#include "network/drivers/driver.hpp"

extern const char op_network_mask[];
extern const char op_network_driver[];
extern const char op_network_op_timeout[];

namespace openperf::network::config {

std::optional<core::cpuset> core_mask();

std::shared_ptr<internal::drivers::driver> driver();

std::chrono::microseconds operation_timeout();

} // namespace openperf::network::config

#endif /* _OP_NETWORK_ARG_PARSER_HPP_ */
