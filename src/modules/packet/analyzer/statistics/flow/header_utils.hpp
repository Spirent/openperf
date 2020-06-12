#ifndef _OP_ANALYZER_STATISTICS_FLOW_HEADER_UTILS_HPP_
#define _OP_ANALYZER_STATISTICS_FLOW_HEADER_UTILS_HPP_

#include <cstdint>
#include <type_traits>

namespace openperf::packet::analyzer::statistics::flow {

template <typename Enum> constexpr auto to_number(Enum e) noexcept
{
    return (static_cast<std::underlying_type_t<Enum>>(e));
}

const uint8_t* skip_ipv6_extension_headers(const uint8_t* cursor,
                                           const uint8_t* end,
                                           uint8_t next_header);

} // namespace openperf::packet::analyzer::statistics::flow

#endif /* _OP_ANALYZER_STATISTICS_FLOW_HEADER_UTILS_HPP_ */
