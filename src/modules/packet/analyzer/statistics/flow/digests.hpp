#ifndef _OP_PACKET_ANALYZER_STATISTICS_FLOW_DIGESTS_HPP_
#define _OP_PACKET_ANALYZER_STATISTICS_FLOW_DIGESTS_HPP_

#include <chrono>

#include "packet/analyzer/statistics/flow/counters.hpp"
#include "packet/statistics/tuple_utils.hpp"
#include "packetio/packet_buffer.hpp"
#include "regurgitate/regurgitate.hpp"

namespace openperf::packet::analyzer::statistics::flow::digest {

inline constexpr size_t digest_size = 16;

template <typename T>
struct digest_impl : public regurgitate::digest<T, float, digest_size>
{
    using mean_type = T;
    using weight_type = float;

    digest_impl()
        : regurgitate::digest<mean_type, weight_type, digest_size>()
    {}

    digest_impl& operator+=(const digest_impl& rhs)
    {
        this->insert(rhs);
        return (*this);
    }

    friend digest_impl operator+(digest_impl lhs, const digest_impl& rhs)
    {
        lhs += rhs;
        return (lhs);
    }
};

struct frame_length final : public digest_impl<float>
{};
struct interarrival final : public digest_impl<float>
{};
struct jitter_ipdv final : public digest_impl<float>
{};
struct jitter_rfc final : public digest_impl<float>
{};
struct latency final : public digest_impl<float>
{};
struct sequence_run_length final : public digest_impl<float>
{};

template <typename T> struct is_duration : std::false_type
{};

template <typename Rep, typename Period>
struct is_duration<std::chrono::duration<Rep, Period>> : std::true_type
{};

template <typename T, typename DigestTuple, typename Value>
inline void update_impl(DigestTuple& tuple, Value v)
{
    using namespace openperf::packet::statistics;

    static_assert(has_type_v<T, DigestTuple>);

    auto& stat = get_counter<T, DigestTuple>(tuple);
    if constexpr (is_duration<Value>::value) {
        stat.insert(
            std::chrono::duration_cast<std::chrono::duration<float, std::nano>>(
                v)
                .count());
    } else {
        stat.insert(v);
    }
}

template <typename T, typename DigestTuple, typename Value>
void update(DigestTuple& tuple, Value&& v)
{
    using namespace openperf::packet::statistics;
    if constexpr (has_type_v<T, DigestTuple>) {
        update_impl<T>(tuple, std::forward<Value>(v));
    }
}

} // namespace openperf::packet::analyzer::statistics::flow::digest

#endif /* _OP_PACKET_ANALYZER_STATISTICS_FLOW_DIGESTS_HPP_ */
