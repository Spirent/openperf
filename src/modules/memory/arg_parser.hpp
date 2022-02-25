#ifndef _OP_MEM_ARG_PARSER_HPP_
#define _OP_MEM_ARG_PARSER_HPP_

#include <optional>

#include "core/op_cpuset.hpp"

namespace openperf::memory::config {

std::optional<core::cpuset> core_mask();

}

#endif /* _OP_MEM_ARG_PARSER_HPP_ */
