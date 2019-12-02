#ifndef _SIMPLE_COUNTER_H_
#define _SIMPLE_COUNTER_H_

#include <atomic>
#include <chrono>
#include <optional>
#include <string>
#include <variant>

struct state_init
{};
struct state_updated
{};

using counter_state = std::variant<state_init, state_updated>;

template <typename Derived, typename T, typename StateVariant>
class counter_state_machine
{
    StateVariant m_state;

public:
    void add(T count, T total)
    {
        Derived& child = static_cast<Derived&>(*this);
        auto next_state = std::visit(
            [&](auto& state) -> std::optional<StateVariant> {
                return (child.on_add(state, count, total));
            },
            m_state);

        if (next_state) m_state = *next_state;
    }
};

template <typename T>
struct simple_counter
    : public counter_state_machine<simple_counter<T>, T, counter_state>
{
    std::atomic<T> count = 0;
    std::atomic<T> total = 0;

    using clock = std::chrono::high_resolution_clock;
    using time_point = std::chrono::time_point<clock>;
    std::atomic<clock::duration> first =
        clock::time_point::max().time_since_epoch();
    std::atomic<clock::duration> last =
        clock::time_point::min().time_since_epoch();

    void reset()
    {
        count.store(0, std::memory_order_release);
        total.store(0, std::memory_order_release);
        first.store(time_point().time_since_epoch(), std::memory_order_release);
        last.store(time_point().time_since_epoch(), std::memory_order_release);
    }

    std::optional<counter_state>
    on_add(const state_init&, T sub_count, T sub_total)
    {
        /* Drop initial arp or whatever */
        if (sub_count == 1) return (std::nullopt);

        first.store(clock::now().time_since_epoch(), std::memory_order_release);
        on_add(state_updated{}, sub_count, sub_total);
        return (state_updated{});
    }

    std::optional<counter_state>
    on_add(const state_updated&, T sub_count, T sub_total)
    {
        count.store(count.load(std::memory_order_consume) + sub_count,
                    std::memory_order_release);
        total.store(total.load(std::memory_order_consume) + sub_total,
                    std::memory_order_release);

        last.store(clock::now().time_since_epoch(), std::memory_order_release);
        return (std::nullopt);
    }
};

#endif /* _SIMPLE_COUNTER_H_ */
