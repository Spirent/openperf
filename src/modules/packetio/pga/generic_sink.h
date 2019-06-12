#ifndef _ICP_PACKETIO_PGA_GENERIC_SINK_H_
#define _ICP_PACKETIO_PGA_GENERIC_SINK_H_

#include <cstdint>
#include <memory>
#include <string>

namespace icp::packetio::pga {

class generic_sink {
public:
    template <typename Sink>
    generic_sink(Sink s)
        : m_self(std::make_shared<sink_model<Sink>>(std::move(s)))
    {}

    std::string id() const
    {
        return m_self->id();
    }

    template <typename PacketType>
    uint16_t push(PacketType* packets[], uint16_t length)
    {
        return m_self->push(reinterpret_cast<void**>(packets), length);
    }

    template <typename PacketType>
    uint16_t push(PacketType* packet)
    {
        return m_self->push(reinterpret_cast<void*>(packet));
    }

private:
    struct sink_concept {
        virtual ~sink_concept() = default;
        virtual std::string id() const = 0;
        virtual uint16_t push(void* packets[], uint16_t length) = 0;
        virtual uint16_t push(void* packet) = 0;
    };

    template <typename Sink>
    struct sink_model final : sink_concept {
        sink_model(Sink s)
            : m_sink(std::move(s))
        {}

        std::string id() const override
        {
            return m_sink.id();
        }

        uint16_t push(void* packets[], uint16_t length) override
        {
            return m_sink.push(packets, length);
        }

        uint16_t push(void* packet) override
        {
            return m_sink.push(packet);
        }

        Sink m_sink;
    };

    std::shared_ptr<sink_concept> m_self;
};

}

#endif /* _ICP_PACKETIO_PGA_GENERIC_SINK_H_ */
