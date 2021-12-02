#ifndef _OP_CPU_ARG_PARSER_HPP_
#define _OP_CPU_ARG_PARSER_HPP_

#include <optional>

#include "core/op_cpuset.hpp"

namespace openperf::cpu::config {

std::optional<core::cpuset> core_mask();

} // namespace openperf::cpu::config

#endif /* _OP_CPU_ARG_PARSER_HPP_ */
