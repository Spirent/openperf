#ifndef _ICP_TDIGEST_H_
#define _ICP_TDIGEST_H_

#include <algorithm>
#include <assert.h>
#include <cmath>
#include <optional>
#include <vector>
#include <atomic>

namespace icp::stats {

// XXX: yes, this could be a std::pair. But being able to refer to values by name
// instead of .first and .second makes merge(), quantile(), and
// cumulative_distribution() way more readable.
template <typename Values = float, typename Weight = unsigned>
struct icp_centroid
{
    Values mean;
    Weight weight;

    icp_centroid(Values new_mean, Weight new_weight)
        : mean(new_mean)
        , weight(new_weight)
    {}

    // friend bool operator<(const icp_centroid &lhs, const icp_centroid &rhs) {
    bool operator<(const icp_centroid<Values, Weight> &rhs) const
    {
        return mean < rhs.mean;
    }

    // friend bool operator>(const icp_centroid &lhs, const icp_centroid &rhs) {
    bool operator>(const icp_centroid<Values, Weight> &rhs) const
    {
        return mean > rhs.mean;
    }
};

template <typename Values = float, typename Weight = unsigned>
class icp_tdigest
{
    using centroid_t = icp_centroid<Values, Weight>;

    struct tdigest_impl {
        std::vector<centroid_t> values;
        Weight total_weight;

        explicit tdigest_impl(size_t size)
            : total_weight(0)
        {
            values.reserve(size);
        }

        tdigest_impl() = delete;

        enum class insert_result { OK, NEED_COMPRESS };

        insert_result insert(Values value, Weight weight)
        {
            assert(weight);
            values.emplace_back(value, weight);
            total_weight += weight;
            return (values.size() != values.capacity() ? insert_result::OK
                                                       : insert_result::NEED_COMPRESS);
        }

        insert_result insert(const centroid_t &val)
        {
            return insert(val.mean, val.weight);
        }

        void reset()
        {
            values.clear();
            total_weight = 0;
        }
    };

    tdigest_impl one;
    tdigest_impl two;

    static constexpr size_t buffer_multiplier = 2;  // FIXME - benchmark for bigger size.
    tdigest_impl buffer;

    std::atomic<tdigest_impl *> active;

    Values min_val, max_val;
    bool run_forward;

public:
    explicit icp_tdigest(size_t size);

    icp_tdigest() = delete;

    /**
     * Inserts the given value into the t-digest input buffer.
     * Assumes a weight of 1 for the sample.
     * If this operation fills the input buffer an automatic merge()
     * operation is triggered.
     *
     * @param value
     *  value to insert
     */
    void insert(Values value)
    {
        insert(value, 1);
    }

    /**
     * Inserts the given value with the given weight into the t-digest input buffer.
     * If this operation fills the input buffer an automatic merge()
     * operation is triggered.
     *
     * @param value
     *  value to insert
     *
     * @param weight
     *  weight associated with the provided value
     */
    void insert(Values value, Weight weight)
    {
        if (buffer.insert(value, weight)
            == tdigest_impl::insert_result::NEED_COMPRESS) {
            merge();
        }
    }

    /**
     * Insert and merge an existing t-digest. Assumes source t-digest
     * is not receiving new data.
     *
     * @param src
     *  source t-digest to copy values from
     */
    void insert(const icp_tdigest<Values, Weight> &src);

    /**
     * Reset internal state of the t-digest.
     *
     * Clears t-digest, incoming data buffer, and resets min/max values.
     */
    void reset();

    /**
     * Retrieve contents of the t-digest.
     *
     * @return
     *  list of <value, weight> pairs sorted by value in ascending order.
     */
    std::vector<std::pair<Values, Weight>> get() const;

    /**
     * Merge incoming data into the t-digest.
     */
    void merge();

    /**
     * Retrieve the probability that a random data point in the digest is
     * less than or equal to x.
     *
     * @param x
     *  value of interest
     *
     * @return
     *  a value between [0, 1]
     */
    double cumulative_distribution(Values x) const;

    /**
     * Retrieve the value such that p percent of all data points or less than or
     * equal to that value.
     *
     * @param p
     *  percentile of interest; valid input between [0.0, 100.0]
     *
     * @return
     *  a value from the dataset that satisfies the above condition
     *  returns -DBL_MAX on error
     */
    double quantile(double p) const;

    /**
     * Return number of centroids in the t-digest
     */
    size_t centroid_count() const
    {
        return ((*active).values.size());
    }

    /**
     * Retrieve the number of merged data points in the t-digest
     *
     * @return
     *   the total weight of all data
     */
    size_t size() const
    {
        return ((*active).total_weight);
    }

    /**
     * Retrieve maximum value seen by this t-digest.
     * Value is cleared by reset().
     *
     * @return
     *  maximum value seen by this t-digest.
     */
    Values max() const
    {
        return (max_val);
    }

    /**
     * Retrieve minimum value seen by this t-digest.
     * Value is cleared by reset().
     *
     * @return
     *  minimum value seen by this t-digest.
     */
    Values min() const
    {
        return (min_val);
    }
};

} // namespace icp::stats::tdigest

#endif
