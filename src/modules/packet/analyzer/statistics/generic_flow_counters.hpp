#ifndef _OP_ANALYZER_STATISTICS_GENERIC_FLOW_COUNTERS_HPP_
#define _OP_ANALYZER_STATISTICS_GENERIC_FLOW_COUNTERS_HPP_

#include <memory>

#include "packet/analyzer/statistics/flow/counters.hpp"
#include "packet/analyzer/statistics/flow/header.hpp"
#include "packet/analyzer/statistics/flow/update.hpp"
#include "packet/analyzer/statistics/generic_flow_digests.hpp"
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
        virtual const generic_flow_digests&
        get(tag<generic_flow_digests>&&) const = 0;
        virtual const flow::counter::frame_counter&
        get(tag<flow::counter::frame_counter>&&) const = 0;
        virtual const flow::counter::frame_length&
        get(tag<flow::counter::frame_length>&&) const = 0;
        virtual const flow::header& get(tag<flow::header>&&) const = 0;
        virtual const flow::counter::interarrival&
        get(tag<flow::counter::interarrival>&&) const = 0;
        virtual const flow::counter::jitter_ipdv&
        get(tag<flow::counter::jitter_ipdv>&&) const = 0;
        virtual const flow::counter::jitter_rfc&
        get(tag<flow::counter::jitter_rfc>&&) const = 0;
        virtual const flow::counter::latency&
        get(tag<flow::counter::latency>&&) const = 0;
        virtual const flow::counter::prbs&
        get(tag<flow::counter::prbs>&&) const = 0;
        virtual const flow::counter::sequencing&
        get(tag<flow::counter::sequencing>&&) const = 0;

        virtual bool holds(tag<generic_flow_digests>&&) const = 0;
        virtual bool holds(tag<flow::counter::frame_counter>&&) const = 0;
        virtual bool holds(tag<flow::counter::frame_length>&&) const = 0;
        virtual bool holds(tag<flow::header>&&) const = 0;
        virtual bool holds(tag<flow::counter::interarrival>&&) const = 0;
        virtual bool holds(tag<flow::counter::latency>&&) const = 0;
        virtual bool holds(tag<flow::counter::jitter_rfc>&&) const = 0;
        virtual bool holds(tag<flow::counter::jitter_ipdv>&&) const = 0;
        virtual bool holds(tag<flow::counter::prbs>&&) const = 0;
        virtual bool holds(tag<flow::counter::sequencing>&&) const = 0;
    };

    template <typename StatsTuple> struct stats_model final : stats_concept
    {
        stats_model(StatsTuple tuple)
            : m_data(std::move(tuple))
        {}

        const generic_flow_digests&
        get(tag<generic_flow_digests>&&) const override
        {
            return (
                packet::statistics::get_counter<generic_flow_digests>(m_data));
        }

        const flow::counter::frame_counter&
        get(tag<flow::counter::frame_counter>&&) const override
        {
            return (
                packet::statistics::get_counter<flow::counter::frame_counter>(
                    m_data));
        }

        const flow::counter::frame_length&
        get(tag<flow::counter::frame_length>&&) const override
        {
            return (
                packet::statistics::get_counter<flow::counter::frame_length>(
                    m_data));
        }

        const flow::header& get(tag<flow::header>&&) const override
        {
            return (packet::statistics::get_counter<flow::header>(m_data));
        }

        const flow::counter::interarrival&
        get(tag<flow::counter::interarrival>&&) const override
        {
            return (
                packet::statistics::get_counter<flow::counter::interarrival>(
                    m_data));
        }

        const flow::counter::jitter_ipdv&
        get(tag<flow::counter::jitter_ipdv>&&) const override
        {
            return (packet::statistics::get_counter<flow::counter::jitter_ipdv>(
                m_data));
        }

        const flow::counter::jitter_rfc&
        get(tag<flow::counter::jitter_rfc>&&) const override
        {
            return (packet::statistics::get_counter<flow::counter::jitter_rfc>(
                m_data));
        }

        const flow::counter::latency&
        get(tag<flow::counter::latency>&&) const override
        {
            return (packet::statistics::get_counter<flow::counter::latency>(
                m_data));
        }

        const flow::counter::prbs&
        get(tag<flow::counter::prbs>&&) const override
        {
            return (
                packet::statistics::get_counter<flow::counter::prbs>(m_data));
        }

        const flow::counter::sequencing&
        get(tag<flow::counter::sequencing>&&) const override
        {
            return (packet::statistics::get_counter<flow::counter::sequencing>(
                m_data));
        }

        bool holds(tag<generic_flow_digests>&&) const override
        {
            return (
                packet::statistics::holds_stat<generic_flow_digests>(m_data));
        }

        bool holds(tag<flow::counter::frame_counter>&&) const override
        {
            return (
                packet::statistics::holds_stat<flow::counter::frame_counter>(
                    m_data));
        }

        bool holds(tag<flow::counter::frame_length>&&) const override
        {
            return (packet::statistics::holds_stat<flow::counter::frame_length>(
                m_data));
        }

        bool holds(tag<flow::header>&&) const override
        {
            return (packet::statistics::holds_stat<flow::header>(m_data));
        }

        bool holds(tag<flow::counter::interarrival>&&) const override
        {
            return (packet::statistics::holds_stat<flow::counter::interarrival>(
                m_data));
        }

        bool holds(tag<flow::counter::jitter_ipdv>&&) const override
        {
            return (packet::statistics::holds_stat<flow::counter::jitter_ipdv>(
                m_data));
        }

        bool holds(tag<flow::counter::jitter_rfc>&&) const override
        {
            return (packet::statistics::holds_stat<flow::counter::jitter_rfc>(
                m_data));
        }

        bool holds(tag<flow::counter::latency>&&) const override
        {
            return (
                packet::statistics::holds_stat<flow::counter::latency>(m_data));
        }

        bool holds(tag<flow::counter::prbs>&&) const override
        {
            return (
                packet::statistics::holds_stat<flow::counter::prbs>(m_data));
        }

        bool holds(tag<flow::counter::sequencing>&&) const override
        {
            return (packet::statistics::holds_stat<flow::counter::sequencing>(
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

        void dump(std::ostream& os) const override
        {
            flow::counter::dump(os, m_data);
        }

        mutable StatsTuple m_data;
    };

    std::shared_ptr<stats_concept> m_self;
};

enum class flow_counter_flags {
    none = 0,
    frame_count = (1 << 0),
    interarrival = (1 << 1),
    frame_length = (1 << 2),
    sequencing = (1 << 3),
    latency = (1 << 4),
    jitter_ipdv = (1 << 5),
    jitter_rfc = (1 << 6),
    prbs = (1 << 7),
    header = (1 << 8),
    digests = (1 << 9),
};

generic_flow_counters
make_flow_counters(openperf::utils::bit_flags<flow_counter_flags> counter_flags,
                   openperf::utils::bit_flags<flow_digest_flags> digest_flags);

enum flow_counter_flags to_flow_counter_flag(std::string_view name);
std::string_view to_name(enum flow_counter_flags);

} // namespace openperf::packet::analyzer::statistics

declare_enum_flags(openperf::packet::analyzer::statistics::flow_counter_flags);

namespace openperf::packet::analyzer::statistics {

inline constexpr auto all_flow_counters =
    (flow_counter_flags::frame_count | flow_counter_flags::frame_length
     | flow_counter_flags::latency | flow_counter_flags::sequencing
     | flow_counter_flags::interarrival | flow_counter_flags::jitter_ipdv
     | flow_counter_flags::jitter_rfc | flow_counter_flags::prbs
     | flow_counter_flags::header | flow_counter_flags::digests);

} // namespace openperf::packet::analyzer::statistics

#endif /* _OP_ANALYZER_STATISTICS_GENERIC_FLOW_COUNTERS_HPP_ */
