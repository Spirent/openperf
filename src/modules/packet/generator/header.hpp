#ifndef _OP_PACKET_GENERATOR_GENERIC_PROTOCOL_HPP_
#define _OP_PACKET_GENERATOR_GENERIC_PROTOCOL_HPP_

#include <memory>
#include <typeindex>

class generic_protocol
{
public:
    template <typename Protocol>
    generic_protocol(Protocol&& h)
        : m_self(std::make_unique<protocol_model<Protocol>>(
            std::forward<Protocol>(h)))
    {}

    template <typename Protocol> Protocol& get()
    {
        if (std::type_index(typeid(Protocol))
            == std::type_index(m_self->type_info())) {
            return (static_cast<protocol_model<Protocol>&>(*m_self).m_protocol);
        }
        throw std::bad_cast();
    }

    uint16_t length() const { return (m_self->length()); }

private:
    struct protocol_concept
    {
        virtual ~protocol_concept() = default;
        virtual const std::type_info& type_info() const = 0;
        virtual uint16_t length() const = 0;
    };

    template <typename Protocol> struct protocol_model final : protocol_concept
    {
        protocol_model(Protocol&& h)
            : m_protocol(std::forward<Protocol>(h))
        {}

        const std::type_info& type_info() const override
        {
            return (typeid(Protocol));
        }

        uint16_t length() const override { return (Protocol::length); }

        Protocol m_protocol;
    };

    std::unique_ptr<protocol_concept> m_self;
};

#endif /* _OP_PACKET_GENERATOR_GENERIC_PROTOCOL_HPP_ */
