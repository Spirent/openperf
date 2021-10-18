#ifndef _OP_CPU_ARG_PARSER_HPP_
#define _OP_CPU_ARG_PARSER_HPP_

#include <bitset>

namespace openperf::cpu::config {

auto constexpr max_core_size = 256;
using core_mask = std::bitset<max_core_size>;

core_mask available_cores();

} // namespace openperf::cpu::config

#endif /* _OP_CPU_ARG_PARSER_HPP_ */
