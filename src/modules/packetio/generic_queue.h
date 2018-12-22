#ifndef _ICP_PACKETIO_GENERIC_QUEUE_H_
#define _ICP_PACKETIO_GENERIC_QUEUE_H_

#include <memory>

namespace icp {
namespace packetio {

class generic_queue {
public:
    template <typename Queue, typename Thing>
    generic_queue(Queue q)
        : m_self(std::make_shared<queue_model<Queue, Thing>>(std::move(q)))
    {}

    template <typename Thing>
    size_t push(const Thing things[], size_t nb_things)
    {
        return (m_self->push(things, nb_things));
    }

    template <typename Thing>
    size_t pull(Thing* things[], size_t max_things)
    {
        return (m_self->pull(things, max_things));
    }

private:
    template <typename Thing>
    struct queue_concept {
        virtual ~queue_concept() = default;
        virtual size_t push(const Thing things[], size_t nb_things) = 0;
        virtual size_t pull(Thing* things[], size_t max_things) = 0;
    };

    template <typename Queue, typename Thing>
    struct queue_model final : queue_concept<Thing> {
        queue_model(Queue q)
            : m_queue(std::move(q))
        {}

        size_t push(const Thing things[], size_t nb_things) override
        {
            return m_queue.push(things, nb_things);
        }

        size_t pull(Thing* things[], size_t max_things) override
        {
            return m_queue.pull(things, max_things);
        }

        Queue m_queue;
    };

    std::shared_ptr<generic_queue> m_self;
};

}
}

#endif /* _ICP_PACKETIO_GENERIC_QUEUE_H_ */
