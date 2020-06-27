#ifndef _OP_DYNAMIC_TDIGEST_HPP_
#define _OP_DYNAMIC_TDIGEST_HPP_

#include <cassert>
#include <cinttypes>

namespace openperf::dynamic {

class tdigest
{
public:
    struct centroid
    {
        uint64_t count = 0;
        double mean = 0.0;

        void update(double x, uint64_t weight)
        {
            count += weight;
            mean += (weight * (x - mean)) / count;
        }
    };

private:
    uint64_t m_weights;
    // double m_compress_factor;

public:
    tdigest() {}
    tdigest(const tdigest& td) {}

    void reset(){};
    void append(double value, uint64_t weight = 1){};

    uint64_t size() const { return m_weights; }
    uint64_t centroids() const { return 0; };
    double quantile(double p) const
    {
        assert(0.0 <= p && p <= 100.0);
        return 0.0;
    }
    double distribution(double x) const { return 0.0; };
};

} // namespace openperf::dynamic

#endif // _OP_DYNAMIC_TDIGEST_HPP_
