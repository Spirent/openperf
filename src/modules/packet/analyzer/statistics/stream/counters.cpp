#include "packet/analyzer/statistics/stream/counters.hpp"

namespace openperf::packet::analyzer::statistics::stream {

void dump(std::ostream& os, const sequencing& stat)
{
    os << "Sequencing:" << std::endl;
    os << " dropped:" << stat.dropped << std::endl;
    os << " duplicate:" << stat.duplicate << std::endl;
    os << " in_order:" << stat.in_order << std::endl;
    os << " late:" << stat.late << std::endl;
    os << " reordered:" << stat.reordered << std::endl;
    os << " run_length:" << stat.run_length << std::endl;
    if (stat.last_seq) {
        os << " last_seq:" << stat.last_seq.value() << std::endl;
    }
}

template <typename SummaryStat>
void dump_duration_stat(std::ostream& os, const SummaryStat& stat)
{
    if (stat.total.count()) {
        os << " min/max:" << stat.min.count() << "/" << stat.max.count()
           << std::endl;
    }
    os << " total: " << stat.total.count() << std::endl;
}

void dump(std::ostream& os, const frame_length& stat)
{
    os << "Frame Length:" << std::endl;
    if (stat.total) {
        os << " min/max:" << stat.min << "/" << stat.max << std::endl;
    }
    os << " total: " << stat.total << std::endl;
}

void dump(std::ostream& os, const interarrival& stat)
{
    os << "Interarrival:" << std::endl;
    dump_duration_stat(os, stat);
}

void dump(std::ostream& os, const jitter_ipdv& stat)
{
    os << "Jitter (IPDV):" << std::endl;
    dump_duration_stat(os, stat);
}

void dump(std::ostream& os, const jitter_rfc& stat)
{
    os << "Jitter (RFC):" << std::endl;
    dump_duration_stat(os, stat);
}

void dump(std::ostream& os, const latency& stat)
{
    os << "Latency:" << std::endl;
    dump_duration_stat(os, stat);
    if (stat.last()) { os << " last: " << stat.last()->count() << std::endl; }
}

} // namespace openperf::packet::analyzer::statistics::stream
