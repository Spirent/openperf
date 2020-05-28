#ifndef _OP_CONFIG_PREFIX_HPP_
#define _OP_CONFIG_PREFIX_HPP_

#include <string>
#include <optional>

namespace openperf::config {
std::optional<std::string> get_prefix();
}

#endif /* _OP_CONFIG_PREFIX_HPP_ */
