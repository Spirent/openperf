#ifndef _ICP_PACKETIO_PGA_GENERIC_SOURCE_H_
#define _ICP_PACKETIO_PGA_GENERIC_SOURCE_H_

#include <cstdint>
#include <memory>
#include <string>

namespace icp::packetio::pga {

class generic_source {
public:
    template <typename Source>
    generic_source(Source s)
        : m_self(std::make_shared<source_model<Source>>(std::move(s)))
    {}

    std::string id() const
    {
        return m_self->id();
    }

    int notification_fd() const
    {
        return m_self->notification_fd();
    }

    unsigned weight() const
    {
        return m_self->weight();
    }

    template <typename PacketType>
    uint16_t pull(PacketType* packets[], uint16_t length)
    {
        return m_self->pull(reinterpret_cast<void**>(packets), length);
    }

    template <typename PacketType>
    uint16_t pull(PacketType* packet)
    {
        return m_self->pull(reinterpret_cast<void*>(packet));
    }

private:
    struct source_concept {
        virtual ~source_concept() = default;
        virtual std::string id() const = 0;
        virtual int notification_fd() const = 0;
        virtual unsigned weight() const = 0;
        virtual uint16_t pull(void* packets[], uint16_t length) = 0;
        virtual uint16_t pull(void* packet) = 0;
    };

    template <typename Source>
    struct source_model final : source_concept {
        source_model(Source s)
            : m_source(std::move(s))
        {}

        std::string id() const override
        {
            return m_source.id();
        }

        int notification_fd() const override
        {
            return m_source.notification_fd();
        }

        unsigned weight() const override
        {
            return m_source.weight();
        }

        uint16_t pull(void* packets[], uint16_t length) override
        {
            return m_source.pull(packets, length);
        }

        uint16_t pull(void* packet) override
        {
            return m_source.pull(packet);
        }

        Source m_source;
    };

    std::shared_ptr<source_concept> m_self;
};

}

#endif /* _ICP_PACKETIO_PGA_GENERIC_SOURCE_H_ */
