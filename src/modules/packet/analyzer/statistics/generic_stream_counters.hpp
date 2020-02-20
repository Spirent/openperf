#ifndef _OP_ANALYZER_STATISTICS_GENERIC_STREAM_COUNTERS_HPP_
#define _OP_ANALYZER_STATISTICS_GENERIC_STREAM_COUNTERS_HPP_

#include <memory>

#include "packet/analyzer/statistics/stream/counters.hpp"
#include "utils/enum_flags.hpp"

namespace openperf::packet::analyzer::statistics {

class generic_stream_counters
{
public:
    template <typename StatsTuple>
    generic_stream_counters(StatsTuple tuple)
        : m_self(std::make_shared<stats_model<StatsTuple>>(std::move(tuple)))
    {}

    template <typename StatsType> const StatsType& get() const
    {
        return (m_self->get<StatsType>());
    }

    template <typename StatsType> StatsType& get()
    {
        return (const_cast<StatsType&>(m_self->get<StatsType>()));
    }

    template <typename StatsType> bool holds() const
    {
        return (m_self->holds<StatsType>());
    }

    void update(uint16_t length,
                counter::timestamp rx,
                std::optional<counter::timestamp> tx = std::nullopt,
                std::optional<uint32_t> seq_num = std::nullopt) const
    {
        m_self->update(length, rx, tx, seq_num);
    }

    void dump(std::ostream& os) const { m_self->dump(os); }

private:
    struct stats_concept
    {
        virtual ~stats_concept() = default;

        template <typename StatsType> struct tag
        {};

        template <typename StatsType> const StatsType& get() const
        {
            return (get(tag<StatsType>{}));
        }

        template <typename StatsType> bool holds() const
        {
            return (holds(tag<StatsType>{}));
        }

        virtual void update(uint16_t length,
                            counter::timestamp rx,
                            std::optional<counter::timestamp> tx,
                            std::optional<uint32_t> seq_num) const = 0;

        virtual void dump(std::ostream& os) const = 0;

    protected:
        virtual const counter& get(tag<counter>&&) const = 0;
        virtual const stream::sequencing&
        get(tag<stream::sequencing>&&) const = 0;
        virtual const stream::frame_length&
        get(tag<stream::frame_length>&&) const = 0;
        virtual const stream::interarrival&
        get(tag<stream::interarrival>&&) const = 0;
        virtual const stream::latency& get(tag<stream::latency>&&) const = 0;
        virtual const stream::jitter_rfc&
        get(tag<stream::jitter_rfc>&&) const = 0;
        virtual const stream::jitter_ipdv&
        get(tag<stream::jitter_ipdv>&&) const = 0;

        virtual bool holds(tag<counter>&&) const = 0;
        virtual bool holds(tag<stream::sequencing>&&) const = 0;
        virtual bool holds(tag<stream::frame_length>&&) const = 0;
        virtual bool holds(tag<stream::interarrival>&&) const = 0;
        virtual bool holds(tag<stream::latency>&&) const = 0;
        virtual bool holds(tag<stream::jitter_rfc>&&) const = 0;
        virtual bool holds(tag<stream::jitter_ipdv>&&) const = 0;
    };

    template <typename StatsTuple> struct stats_model final : stats_concept
    {
        stats_model(StatsTuple tuple)
            : m_data(std::move(tuple))
        {}

        const counter& get(tag<counter>&&) const override
        {
            return (get_counter<counter, StatsTuple>(m_data));
        }

        const stream::sequencing& get(tag<stream::sequencing>&&) const override
        {
            return (get_counter<stream::sequencing, StatsTuple>(m_data));
        }

        const stream::frame_length&
        get(tag<stream::frame_length>&&) const override
        {
            return (get_counter<stream::frame_length, StatsTuple>(m_data));
        }

        const stream::interarrival&
        get(tag<stream::interarrival>&&) const override
        {
            return (get_counter<stream::interarrival, StatsTuple>(m_data));
        }

        const stream::latency& get(tag<stream::latency>&&) const override
        {
            return (get_counter<stream::latency, StatsTuple>(m_data));
        }

        const stream::jitter_rfc& get(tag<stream::jitter_rfc>&&) const override
        {
            return (get_counter<stream::jitter_rfc, StatsTuple>(m_data));
        }

        const stream::jitter_ipdv&
        get(tag<stream::jitter_ipdv>&&) const override
        {
            return (get_counter<stream::jitter_ipdv, StatsTuple>(m_data));
        }

        bool holds(tag<counter>&&) const override
        {
            return (holds_stat<counter, StatsTuple>(m_data));
        }

        bool holds(tag<stream::sequencing>&&) const override
        {
            return (holds_stat<stream::sequencing, StatsTuple>(m_data));
        }

        bool holds(tag<stream::frame_length>&&) const override
        {
            return (holds_stat<stream::frame_length, StatsTuple>(m_data));
        }

        bool holds(tag<stream::interarrival>&&) const override
        {
            return (holds_stat<stream::interarrival, StatsTuple>(m_data));
        }

        bool holds(tag<stream::latency>&&) const override
        {
            return (holds_stat<stream::latency, StatsTuple>(m_data));
        }

        bool holds(tag<stream::jitter_rfc>&&) const override
        {
            return (holds_stat<stream::jitter_rfc, StatsTuple>(m_data));
        }

        bool holds(tag<stream::jitter_ipdv>&&) const override
        {
            return (holds_stat<stream::jitter_ipdv, StatsTuple>(m_data));
        }

        void update(uint16_t length,
                    counter::timestamp rx,
                    std::optional<counter::timestamp> tx,
                    std::optional<uint32_t> seq_num) const override
        {
            stream::update(m_data, length, rx, tx, seq_num);
        }

        void dump(std::ostream& os) const override { stream::dump(os, m_data); }

        mutable StatsTuple m_data;
    };

    std::shared_ptr<stats_concept> m_self;
};

enum class stream_flags {
    none = 0,
    frame_count = (1 << 0),
    interarrival = (1 << 1),
    frame_length = (1 << 2),
    sequencing = (1 << 3),
    latency = (1 << 4),
    jitter_ipdv = (1 << 5),
    jitter_rfc = (1 << 6),
    prbs = (1 << 7),
};

generic_stream_counters
make_counters(openperf::utils::bit_flags<stream_flags> flags);

enum stream_flags to_stream_flag(std::string_view name);
std::string_view to_name(enum stream_flags);

} // namespace openperf::packet::analyzer::statistics

declare_enum_flags(openperf::packet::analyzer::statistics::stream_flags);

namespace openperf::packet::analyzer::statistics {
inline constexpr auto all_stream_counters =
    (stream_flags::frame_count | stream_flags::frame_length
     | stream_flags::latency | stream_flags::sequencing
     | stream_flags::interarrival | stream_flags::jitter_ipdv
     | stream_flags::jitter_rfc);
}

#endif /* _OP_ANALYZER_STATISTICS_GENERIC_STREAM_COUNTERS_HPP_ */
