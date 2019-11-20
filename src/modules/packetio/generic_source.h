#ifndef _OP_PACKETIO_GENERIC_SOURCE_H_
#define _OP_PACKETIO_GENERIC_SOURCE_H_

#include <any>
#include <memory>
#include <string>

#include "units/rate.h"

namespace openperf::packetio::packets {

struct packet_buffer;

using packets_per_hour   = openperf::units::rate<uint64_t, std::ratio<1, 3600>>;

class generic_source {
public:
    template <typename Source>
    generic_source(Source s)
        : m_self(std::make_shared<source_model<Source>>(std::move(s)))
    {}

    std::string id() const
    {
        return (m_self->id());
    }

    bool active() const
    {
        return (m_self->active());
    }

    uint16_t burst_size() const
    {
        return (m_self->burst_size());
    }

    uint16_t max_packet_length() const
    {
        return (m_self->max_packet_length());
    }

    /*
     * Packets... Per... Hour ?!?!?
     * Yep.  Rationale is that it allows us to send traffic at less than
     * 1 fps and keep all units (and operations on them) integer based.
     */
    packets_per_hour packet_rate() const
    {
        return (m_self->packet_rate());
    }

    uint16_t transform(packet_buffer* input[], uint16_t input_length,
                       packet_buffer* output[]) const
    {
        return (m_self->transform(input, input_length, output));
    }

    bool operator==(const generic_source& other) const
    {
        return (id() == other.id());
    }

    template <typename Source>
    Source& get() const
    {
        return (*(std::any_cast<Source*>(m_self->get_pointer())));
    }

private:
    struct source_concept {
        virtual ~source_concept() = default;
        virtual std::string id() const = 0;
        virtual bool active() const = 0;
        virtual uint16_t burst_size() const = 0;
        virtual uint16_t max_packet_length() const = 0;
        virtual packets_per_hour packet_rate() const = 0;
        virtual uint16_t transform(packet_buffer* input[], uint16_t input_length,
                                   packet_buffer* output[]) = 0;
        virtual std::any get_pointer() noexcept = 0;
    };

    /**
     *  SFINAE template structs to determine optional functionality.
     **/

    /* bool active(); */
    template <typename T, typename = std::void_t<> >
    struct has_active : std::false_type {};

    template <typename T>
    struct has_active<T, std::void_t<decltype(std::declval<T>().active())>>
        : std::true_type {};

    /* uint16_t burst_size(); */
    template <typename T, typename = std::void_t<> >
    struct has_burst_size : std::false_type {};

    template <typename T>
    struct has_burst_size<T, std::void_t<decltype(std::declval<T>().burst_size())>>
        : std::true_type {};

    /* uint16_t max_packet_length */
    template <typename T, typename = std::void_t<> >
    struct has_max_packet_length : std::false_type {};

    template <typename T>
    struct has_max_packet_length<T, std::void_t<decltype(std::declval<T>().max_packet_length())>>
        : std::true_type {};

    template <typename Source>
    struct source_model final : source_concept {
        source_model(Source s)
            : m_source(std::move(s))
        {}

        std::string id() const override
        {
            return (m_source.id());
        }

        bool active() const override
        {
            if constexpr (has_active<Source>::value) {
                return (m_source.active());
            } else {
                return (true);
            }
        }

        uint16_t burst_size() const override
        {
            if constexpr (has_burst_size<Source>::value) {
                return (m_source.burst_size());
            } else {
                return (1);
            }
        }

        uint16_t max_packet_length() const override
        {
            if constexpr (has_max_packet_length<Source>::value) {
                return (m_source.max_packet_length());
            } else {
                return (1500);
            }
        }

        packets_per_hour packet_rate() const override
        {
            return (m_source.packet_rate());
        }

        uint16_t transform(packet_buffer* input[], uint16_t input_length,
                           packet_buffer* output[]) override
        {
            return (m_source.transform(input, input_length, output));
        }

        std::any get_pointer() noexcept override
        {
            return (std::addressof(m_source));
        }

        Source m_source;
    };

    std::shared_ptr<source_concept> m_self;
};

}

#endif /* _OP_PACKETIO_GENERIC_SOURCE_H_ */
