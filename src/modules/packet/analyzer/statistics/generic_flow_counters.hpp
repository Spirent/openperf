#ifndef _OP_ANALYZER_STATISTICS_GENERIC_FLOW_COUNTERS_HPP_
#define _OP_ANALYZER_STATISTICS_GENERIC_FLOW_COUNTERS_HPP_

#include <memory>

#include "packet/analyzer/statistics/flow/counters.hpp"
#include "packet/analyzer/statistics/flow/header.hpp"
#include "utils/enum_flags.hpp"

namespace openperf::packet::analyzer::statistics {

class generic_flow_counters
{
    using packet_type_flags = flow::header::packet_type_flags;

public:
    template <typename StatsTuple>
    generic_flow_counters(StatsTuple tuple)
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

    void set_header(const packetio::packet::packet_buffer* pkt) const
    {
        m_self->set_header(pkt);
    }

    void update(const packetio::packet::packet_buffer* pkt) const
    {
        m_self->update(pkt);
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

        virtual void
        set_header(const packetio::packet::packet_buffer* pkt) const = 0;

        virtual void
        update(const packetio::packet::packet_buffer* pkt) const = 0;

        virtual void dump(std::ostream& os) const = 0;

    protected:
        virtual const flow::errors& get(tag<flow::errors>&&) const = 0;
        virtual const flow::frame_counter&
        get(tag<flow::frame_counter>&&) const = 0;
        virtual const flow::frame_length&
        get(tag<flow::frame_length>&&) const = 0;
        virtual const flow::header& get(tag<flow::header>&&) const = 0;
        virtual const flow::interarrival&
        get(tag<flow::interarrival>&&) const = 0;
        virtual const flow::jitter_ipdv&
        get(tag<flow::jitter_ipdv>&&) const = 0;
        virtual const flow::jitter_rfc& get(tag<flow::jitter_rfc>&&) const = 0;
        virtual const flow::latency& get(tag<flow::latency>&&) const = 0;
        virtual const flow::prbs& get(tag<flow::prbs>&&) const = 0;
        virtual const flow::sequencing& get(tag<flow::sequencing>&&) const = 0;

        virtual bool holds(tag<flow::errors>&&) const = 0;
        virtual bool holds(tag<flow::frame_counter>&&) const = 0;
        virtual bool holds(tag<flow::frame_length>&&) const = 0;
        virtual bool holds(tag<flow::header>&&) const = 0;
        virtual bool holds(tag<flow::interarrival>&&) const = 0;
        virtual bool holds(tag<flow::latency>&&) const = 0;
        virtual bool holds(tag<flow::jitter_rfc>&&) const = 0;
        virtual bool holds(tag<flow::jitter_ipdv>&&) const = 0;
        virtual bool holds(tag<flow::prbs>&&) const = 0;
        virtual bool holds(tag<flow::sequencing>&&) const = 0;
    };

    template <typename StatsTuple> struct stats_model final : stats_concept
    {
        stats_model(StatsTuple tuple)
            : m_data(std::move(tuple))
        {}

        const flow::errors& get(tag<flow::errors>&&) const override
        {
            return (packet::statistics::get_counter<flow::errors, StatsTuple>(
                m_data));
        }

        const flow::frame_counter&
        get(tag<flow::frame_counter>&&) const override
        {
            return (packet::statistics::get_counter<flow::frame_counter,
                                                    StatsTuple>(m_data));
        }

        const flow::frame_length& get(tag<flow::frame_length>&&) const override
        {
            return (
                packet::statistics::get_counter<flow::frame_length, StatsTuple>(
                    m_data));
        }

        const flow::header& get(tag<flow::header>&&) const override
        {
            return (packet::statistics::get_counter<flow::header, StatsTuple>(
                m_data));
        }

        const flow::interarrival& get(tag<flow::interarrival>&&) const override
        {
            return (
                packet::statistics::get_counter<flow::interarrival, StatsTuple>(
                    m_data));
        }

        const flow::jitter_ipdv& get(tag<flow::jitter_ipdv>&&) const override
        {
            return (
                packet::statistics::get_counter<flow::jitter_ipdv, StatsTuple>(
                    m_data));
        }

        const flow::jitter_rfc& get(tag<flow::jitter_rfc>&&) const override
        {
            return (
                packet::statistics::get_counter<flow::jitter_rfc, StatsTuple>(
                    m_data));
        }

        const flow::latency& get(tag<flow::latency>&&) const override
        {
            return (packet::statistics::get_counter<flow::latency, StatsTuple>(
                m_data));
        }

        const flow::prbs& get(tag<flow::prbs>&&) const override
        {
            return (packet::statistics::get_counter<flow::prbs, StatsTuple>(
                m_data));
        }

        const flow::sequencing& get(tag<flow::sequencing>&&) const override
        {
            return (
                packet::statistics::get_counter<flow::sequencing, StatsTuple>(
                    m_data));
        }

        bool holds(tag<flow::errors>&&) const override
        {
            return (packet::statistics::holds_stat<flow::errors, StatsTuple>(
                m_data));
        }

        bool holds(tag<flow::frame_counter>&&) const override
        {
            return (
                packet::statistics::holds_stat<flow::frame_counter, StatsTuple>(
                    m_data));
        }

        bool holds(tag<flow::frame_length>&&) const override
        {
            return (
                packet::statistics::holds_stat<flow::frame_length, StatsTuple>(
                    m_data));
        }

        bool holds(tag<flow::header>&&) const override
        {
            return (packet::statistics::holds_stat<flow::header, StatsTuple>(
                m_data));
        }

        bool holds(tag<flow::interarrival>&&) const override
        {
            return (
                packet::statistics::holds_stat<flow::interarrival, StatsTuple>(
                    m_data));
        }

        bool holds(tag<flow::jitter_ipdv>&&) const override
        {
            return (
                packet::statistics::holds_stat<flow::jitter_ipdv, StatsTuple>(
                    m_data));
        }

        bool holds(tag<flow::jitter_rfc>&&) const override
        {
            return (
                packet::statistics::holds_stat<flow::jitter_rfc, StatsTuple>(
                    m_data));
        }

        bool holds(tag<flow::latency>&&) const override
        {
            return (packet::statistics::holds_stat<flow::latency, StatsTuple>(
                m_data));
        }

        bool holds(tag<flow::prbs>&&) const override
        {
            return (
                packet::statistics::holds_stat<flow::prbs, StatsTuple>(m_data));
        }

        bool holds(tag<flow::sequencing>&&) const override
        {
            return (
                packet::statistics::holds_stat<flow::sequencing, StatsTuple>(
                    m_data));
        }

        void
        set_header(const packetio::packet::packet_buffer* pkt) const override
        {
            flow::set_header(m_data, pkt);
        }

        void update(const packetio::packet::packet_buffer* pkt) const override
        {
            flow::update(m_data, pkt);
        }

        void dump(std::ostream& os) const override { flow::dump(os, m_data); }

        mutable StatsTuple m_data;
    };

    std::shared_ptr<stats_concept> m_self;
};

enum class flow_flags {
    none = 0,
    frame_count = (1 << 0),
    interarrival = (1 << 1),
    frame_length = (1 << 2),
    sequencing = (1 << 3),
    latency = (1 << 4),
    jitter_ipdv = (1 << 5),
    jitter_rfc = (1 << 6),
    prbs = (1 << 7),
    errors = (1 << 8),
    header = (1 << 9),
};

generic_flow_counters
make_counters(openperf::utils::bit_flags<flow_flags> flags);

enum flow_flags to_flow_flag(std::string_view name);
std::string_view to_name(enum flow_flags);

} // namespace openperf::packet::analyzer::statistics

declare_enum_flags(openperf::packet::analyzer::statistics::flow_flags);

namespace openperf::packet::analyzer::statistics {
inline constexpr auto all_flow_counters =
    (flow_flags::frame_count | flow_flags::frame_length | flow_flags::latency
     | flow_flags::sequencing | flow_flags::interarrival
     | flow_flags::jitter_ipdv | flow_flags::jitter_rfc | flow_flags::prbs
     | flow_flags::errors | flow_flags::header);
}

#endif /* _OP_ANALYZER_STATISTICS_GENERIC_FLOW_COUNTERS_HPP_ */
