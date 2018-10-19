#ifndef _ICP_PACKETIO_GENERIC_STACK_H_
#define _ICP_PACKETIO_GENERIC_STACK_H_

#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "tl/expected.hpp"

#include "packetio/generic_driver.h"
#include "packetio/generic_interface.h"

namespace icp {
namespace packetio {
namespace stack {

class generic_stack {
public:
    template <typename Stack>
    generic_stack(Stack s)
        : m_self(std::make_unique<stack_model<Stack>>(std::move(s)))
    {}

    std::vector<int> interface_ids() const
    {
        return m_self->interface_ids();
    }

    std::optional<interface::generic_interface> interface(int id) const
    {
        return m_self->interface(id);
    }

    tl::expected<int, std::string> create_interface(int port_id,
                                                    const interface::config_data& config)
    {
        return m_self->create_interface(port_id, config);
    }

    void delete_interface(int id)
    {
        m_self->delete_interface(id);
    }

private:
    struct stack_concept {
        virtual ~stack_concept() = default;
        virtual std::vector<int> interface_ids() const = 0;
        virtual std::optional<interface::generic_interface> interface(int id) const = 0;
        virtual tl::expected<int, std::string> create_interface(int port_id,
                                                                const interface::config_data& config) = 0;
        virtual void delete_interface(int id) = 0;
    };

    template <typename Stack>
    struct stack_model final : stack_concept {
        stack_model(Stack s)
            : m_stack(std::move(s))
        {}

        std::vector<int> interface_ids() const
        {
            return m_stack.interface_ids();
        }

        std::optional<interface::generic_interface> interface(int id) const
        {
            return m_stack.interface(id);
        }

        tl::expected<int, std::string> create_interface(int port_id,
                                                        const interface::config_data& config)
        {
            return m_stack.create_interface(port_id, config);
        }

        void delete_interface(int id)
        {
            m_stack.delete_interface(id);
        }

        Stack m_stack;
    };

    std::unique_ptr<stack_concept> m_self;
};

std::unique_ptr<generic_stack> make(driver::generic_driver& driver);

}
}
}
#endif /* _ICP_PATCKETIO_GENERIC_STACK_H_ */
