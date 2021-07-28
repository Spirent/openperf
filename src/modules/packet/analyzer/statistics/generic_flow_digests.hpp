#ifndef _OP_ANALYZER_STATISTICS_GENERIC_FLOW_DIGESTS_HPP_
#define _OP_ANALYZER_STATISTICS_GENERIC_FLOW_DIGESTS_HPP_

#include "packet/analyzer/statistics/flow/counters.hpp"
#include "packet/analyzer/statistics/flow/digests.hpp"
#include "utils/enum_flags.hpp"

namespace openperf::packet::analyzer::statistics {

class generic_flow_digests
{
public:
    template <typename DigestsTuple>
    generic_flow_digests(DigestsTuple tuple)
        : m_self(
            std::make_shared<digests_model<DigestsTuple>>(std::move(tuple)))
    {}

    template <typename DigestType> DigestType& get()
    {
        return (const_cast<DigestType&>(m_self->get<DigestType>()));
    }

    template <typename DigestType> const DigestType& get() const
    {
        return (m_self->get<DigestType>());
    }

    template <typename DigestType> bool holds() const
    {
        return (m_self->holds<DigestType>());
    }

    template <typename DigestType, typename ValueType>
    void update(ValueType v) const
    {
        m_self->update<DigestType>(v);
    }

    void write_prefetch() const { __builtin_prefetch(m_self.get(), 1, 0); }

private:
    struct digests_concept
    {
        virtual ~digests_concept() = default;

        template <typename DigestType> struct tag
        {};

        template <typename DigestType> const DigestType& get() const
        {
            return (get(tag<DigestType>{}));
        }

        template <typename DigestType> bool holds() const
        {
            return (holds(tag<DigestType>{}));
        }

        template <typename DigestType, typename ValueType>
        void update(ValueType v) const
        {
            update(tag<DigestType>{}, v);
        }

    protected:
        virtual const flow::digest::interarrival&
        get(tag<flow::digest::interarrival>&&) const = 0;
        virtual const flow::digest::frame_length&
        get(tag<flow::digest::frame_length>&&) const = 0;
        virtual const flow::digest::jitter_ipdv&
        get(tag<flow::digest::jitter_ipdv>&&) const = 0;
        virtual const flow::digest::jitter_rfc&
        get(tag<flow::digest::jitter_rfc>&&) const = 0;
        virtual const flow::digest::latency&
        get(tag<flow::digest::latency>&&) const = 0;
        virtual const flow::digest::sequence_run_length&
        get(tag<flow::digest::sequence_run_length>&&) const = 0;

        virtual bool holds(tag<flow::digest::interarrival>&&) const = 0;
        virtual bool holds(tag<flow::digest::frame_length>&&) const = 0;
        virtual bool holds(tag<flow::digest::jitter_ipdv>&&) const = 0;
        virtual bool holds(tag<flow::digest::jitter_rfc>&&) const = 0;
        virtual bool holds(tag<flow::digest::latency>&&) const = 0;
        virtual bool holds(tag<flow::digest::sequence_run_length>&&) const = 0;

        virtual void update(tag<flow::digest::interarrival>&&,
                            flow::counter::interarrival::pop_t v) const = 0;
        virtual void update(tag<flow::digest::frame_length>&&,
                            flow::counter::frame_length::pop_t v) const = 0;
        virtual void update(tag<flow::digest::jitter_ipdv>&&,
                            flow::counter::jitter_ipdv::pop_t v) const = 0;
        virtual void update(tag<flow::digest::jitter_rfc>&&,
                            flow::counter::jitter_rfc::pop_t v) const = 0;
        virtual void update(tag<flow::digest::latency>&&,
                            flow::counter::latency::pop_t v) const = 0;
        virtual void
        update(tag<flow::digest::sequence_run_length>&&,
               flow::digest::sequence_run_length::mean_type v) const = 0;
    };

    template <typename DigestsTuple>
    struct digests_model final : digests_concept
    {
        digests_model(DigestsTuple tuple)
            : m_data(std::move(tuple))
        {}

        const flow::digest::interarrival&
        get(tag<flow::digest::interarrival>&&) const override
        {
            return (packet::statistics::get_counter<flow::digest::interarrival,
                                                    DigestsTuple>(m_data));
        }

        const flow::digest::frame_length&
        get(tag<flow::digest::frame_length>&&) const override
        {
            return (packet::statistics::get_counter<flow::digest::frame_length,
                                                    DigestsTuple>(m_data));
        }
        const flow::digest::jitter_ipdv&
        get(tag<flow::digest::jitter_ipdv>&&) const override
        {
            return (packet::statistics::get_counter<flow::digest::jitter_ipdv,
                                                    DigestsTuple>(m_data));
        }
        const flow::digest::jitter_rfc&
        get(tag<flow::digest::jitter_rfc>&&) const override
        {
            return (packet::statistics::get_counter<flow::digest::jitter_rfc,
                                                    DigestsTuple>(m_data));
        }
        const flow::digest::latency&
        get(tag<flow::digest::latency>&&) const override
        {
            return (packet::statistics::get_counter<flow::digest::latency,
                                                    DigestsTuple>(m_data));
        }

        const flow::digest::sequence_run_length&
        get(tag<flow::digest::sequence_run_length>&&) const override
        {
            return (packet::statistics::get_counter<
                    flow::digest::sequence_run_length,
                    DigestsTuple>(m_data));
        }

        bool holds(tag<flow::digest::interarrival>&&) const override
        {
            return (packet::statistics::holds_stat<flow::digest::interarrival>(
                m_data));
        }

        bool holds(tag<flow::digest::frame_length>&&) const override
        {
            return (packet::statistics::holds_stat<flow::digest::frame_length>(
                m_data));
        }
        bool holds(tag<flow::digest::jitter_ipdv>&&) const override
        {
            return (packet::statistics::holds_stat<flow::digest::jitter_ipdv>(
                m_data));
        }
        bool holds(tag<flow::digest::jitter_rfc>&&) const override
        {
            return (packet::statistics::holds_stat<flow::digest::jitter_rfc>(
                m_data));
        }
        bool holds(tag<flow::digest::latency>&&) const override
        {
            return (
                packet::statistics::holds_stat<flow::digest::latency>(m_data));
        }
        bool holds(tag<flow::digest::sequence_run_length>&&) const override
        {
            return (packet::statistics::holds_stat<
                    flow::digest::sequence_run_length>(m_data));
        }

        void update(tag<flow::digest::interarrival>&&,
                    flow::counter::interarrival::pop_t v) const override
        {
            flow::digest::update<flow::digest::interarrival>(m_data, v);
        }
        void update(tag<flow::digest::frame_length>&&,
                    flow::counter::frame_length::pop_t v) const override
        {
            flow::digest::update<flow::digest::frame_length>(m_data, v);
        }
        void update(tag<flow::digest::jitter_ipdv>&&,
                    flow::counter::jitter_ipdv::pop_t v) const override
        {
            flow::digest::update<flow::digest::jitter_ipdv>(m_data, v);
        }
        void update(tag<flow::digest::jitter_rfc>&&,
                    flow::counter::jitter_rfc::pop_t v) const override
        {
            flow::digest::update<flow::digest::jitter_rfc>(m_data, v);
        }
        void update(tag<flow::digest::latency>&&,
                    flow::counter::latency::pop_t v) const override
        {
            flow::digest::update<flow::digest::latency>(m_data, v);
        }
        void
        update(tag<flow::digest::sequence_run_length>&&,
               flow::digest::sequence_run_length::mean_type v) const override
        {
            flow::digest::update<flow::digest::sequence_run_length>(m_data, v);
        }

        mutable DigestsTuple m_data;
    };

    std::shared_ptr<digests_concept> m_self;
};

enum class flow_digest_flags {
    none = 0,
    frame_length = (1 << 0),
    interarrival = (1 << 1),
    jitter_ipdv = (1 << 2),
    jitter_rfc = (1 << 3),
    latency = (1 << 4),
    sequence_run_length = (1 << 5)
};

generic_flow_digests
make_flow_digests(openperf::utils::bit_flags<flow_digest_flags> flags);

enum flow_digest_flags to_flow_digest_flag(std::string_view name);
std::string_view to_name(enum flow_digest_flags);

} // namespace openperf::packet::analyzer::statistics

declare_enum_flags(openperf::packet::analyzer::statistics::flow_digest_flags);

namespace openperf::packet::analyzer::statistics {
inline constexpr auto all_flow_digests =
    (flow_digest_flags::interarrival | flow_digest_flags::frame_length
     | flow_digest_flags::jitter_ipdv | flow_digest_flags::jitter_rfc
     | flow_digest_flags::latency | flow_digest_flags::sequence_run_length);
}

#endif /* _OP_ANALYZER_STATISTICS_GENERIC_FLOW_DIGESTS_HPP_ */
