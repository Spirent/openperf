#ifndef _OP_PACKETIO_GENERIC_SINK_HPP_
#define _OP_PACKETIO_GENERIC_SINK_HPP_

#include <any>
#include <memory>
#include <string>
#include <typeindex>

#include "utils/enum_flags.hpp"

namespace openperf::packetio::packet {

struct packet_buffer;

enum class sink_feature_flags {
    none = 0,
    rx_timestamp = (1 << 0),
    spirent_signature_decode = (1 << 1),
};

class generic_sink
{
public:
    template <typename Sink>
    generic_sink(Sink s)
        : m_self(std::make_shared<sink_model<Sink>>(std::move(s)))
    {}

    std::string id() const { return (m_self->id()); }

    bool active() const { return (m_self->active()); }

    uint16_t push(packet_buffer* const packets[], uint16_t length) const
    {
        return (m_self->push(packets, length));
    }

    bool uses_feature(enum sink_feature_flags flags) const
    {
        return (m_self->uses_feature(flags));
    }

    bool operator==(const generic_sink& other) const
    {
        return (id() == other.id());
    }

    template <typename Sink> Sink& get() const
    {
        if (std::type_index(typeid(Sink))
            == std::type_index(m_self->type_info())) {
            return (static_cast<sink_model<Sink>&>(*m_self).m_sink);
        }
        throw std::bad_cast();
    }

private:
    struct sink_concept
    {
        virtual ~sink_concept() = default;
        virtual std::string id() const = 0;
        virtual bool active() const = 0;
        virtual uint16_t push(packet_buffer* const packets[],
                              uint16_t length) = 0;
        virtual bool uses_feature(enum sink_feature_flags) const = 0;
        virtual const std::type_info& type_info() const = 0;
    };

    template <typename Sink> struct sink_model final : sink_concept
    {
        sink_model(Sink s)
            : m_sink(std::move(s))
        {}

        std::string id() const override { return (m_sink.id()); }

        bool active() const override { return (m_sink.active()); }

        uint16_t push(packet_buffer* const packets[], uint16_t length) override
        {
            return (m_sink.push(packets, length));
        }

        bool uses_feature(enum sink_feature_flags flags) const override
        {
            return (m_sink.uses_feature(flags));
        }

        const std::type_info& type_info() const override
        {
            return (typeid(Sink));
        }

        Sink m_sink;
    };

    std::shared_ptr<sink_concept> m_self;
};

} // namespace openperf::packetio::packet

declare_enum_flags(openperf::packetio::packet::sink_feature_flags);

#endif /* _OP_PACKETIO_GENERIC_SINK_HPP_ */
