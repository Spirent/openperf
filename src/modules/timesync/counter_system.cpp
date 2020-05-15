#include <chrono>

#include "timesync/counter.hpp"

namespace openperf::timesync::counter {

class system final : public timecounter::registrar<system>
{
    using clock = std::chrono::steady_clock;

public:
    system()
        : timecounter::registrar<system>()
    {}

    std::string_view name() const override { return "system"; }

    ticks now() const override
    {
        return (clock::now().time_since_epoch().count());
    }

    hz frequency() const override
    {
        using namespace std::chrono_literals;
        return (units::to_rate<hz>(clock::duration{1}));
    }

    static constexpr int priority = 100;

    int static_priority() const override { return (priority); }
};

} // namespace openperf::timesync::counter
