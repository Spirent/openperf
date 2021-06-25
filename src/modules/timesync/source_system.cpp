#include <thread>

#include "core/op_event_loop.hpp"
#include "timesync/clock.hpp"
#include "timesync/source_system.hpp"

namespace openperf::timesync::source {

using sys_clock = std::chrono::system_clock;
static constexpr auto system_poll_period = 32s;

std::chrono::nanoseconds system_poll_delay(unsigned i,
                                           std::chrono::seconds max_period)
{
    using seconds = std::chrono::duration<double>;
    static constexpr int system_startup_polls = 8;

    auto period = seconds(
        i < system_startup_polls ? std::pow(
            std::exp(std::log(max_period.count()) / system_startup_polls), i)
                                 : max_period.count());

    return (std::chrono::duration_cast<std::chrono::nanoseconds>(period));
}

static int handle_system_poll(const struct op_event_data* data, void* arg)
{
    auto* source = reinterpret_cast<source::system*>(arg);

    /* Generate the four timestamps needed to sync the clock */
    auto tx1 = counter::now();
    auto rx1 = to_bintime(sys_clock::now().time_since_epoch());

    /*
     * XXX: Need different values for rx1 and tx2, so sleep for a
     * minimum of one clock tick.
     */
    using sys_ticks = std::chrono::duration<sys_clock::rep, sys_clock::period>;
    std::this_thread::sleep_for(sys_ticks{1});

    auto tx2 = to_bintime(sys_clock::now().time_since_epoch());
    auto rx2 = counter::now();

    assert(rx1 < tx2);
    assert(tx1 < rx2);

    source->poll_count++;
    source->clock->update(tx1, rx1, tx2, rx2);

    op_event_loop_update(
        data->loop,
        source->poll_loop_id,
        static_cast<uint64_t>(
            system_poll_delay(source->poll_count, system_poll_period).count()));

    return (0);
}

system::system(core::event_loop& loop_, class clock* clock_)
    : loop(loop_)
    , clock(clock_)
{
    auto callbacks = op_event_callbacks{.on_timeout = handle_system_poll};
    loop.get().add((100ns).count(), &callbacks, this, &poll_loop_id);
    assert(poll_loop_id);
}

system::~system()
{
    assert(poll_loop_id);
    loop.get().del(poll_loop_id);
}

api::time_source_system to_time_source(const system& source)
{
    auto sys = api::time_source_system{
        .stats = {
            .poll_count = source.poll_count,
            .poll_period = std::chrono::duration_cast<std::chrono::seconds>(
                system_poll_delay(source.poll_count, system_poll_period))}};

    return (sys);
}

} // namespace openperf::timesync::source
