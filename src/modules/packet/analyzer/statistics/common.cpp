#include "packet/analyzer/statistics/common.hpp"

namespace openperf::packet::analyzer::statistics {

void dump(std::ostream& os, const counter& counter, std::string_view name)
{
    os << name << ":" << std::endl;
    if (counter.count) {
        os << " first:" << counter.first()->time_since_epoch().count()
           << std::endl;
        os << " last:" << counter.last()->time_since_epoch().count()
           << std::endl;
    }
    os << " count: " << counter.count << std::endl;
}

} // namespace openperf::packet::analyzer::statistics
