//FIXME - add attribution to tdunning implementation.


// Hello there. If you're reading this you probably want to #include
// this file to specialize the template for different value/weight types.
// May we instead suggest adding a new entry in icp_centroid.cpp
// with those different types?

#include "icp_tdigest.h"

namespace icp::stats {

template <typename Values, typename Weight>
icp_tdigest<Values, Weight>::icp_tdigest(size_t size)
    : one(size)
    , two(size)
    , buffer(size * buffer_multiplier)
    , active(&one)
    , min_val(std::numeric_limits<Values>::max())
    , max_val(std::numeric_limits<Values>::lowest())
    , run_forward(true)
{}

template <typename Values, typename Weight>
void icp_tdigest<Values, Weight>::insert(const icp_tdigest<Values, Weight> &src)
{
    auto insert_fn = [this](const auto &val){
                         if (buffer.insert(val) ==
                             tdigest_impl::insert_result::NEED_COMPRESS) {
                             merge();
                         }
                     };

    std::for_each((*src.active).values.begin(), (*src.active).values.end(), insert_fn);
    // Explicitly merge any unmerged data for a consistent end state.
    merge();
}

template <typename Values, typename Weight>
void icp_tdigest<Values, Weight>::reset()
{
    one.reset();
    two.reset();
    buffer.reset();
    active = &one;
    min_val = std::numeric_limits<Values>::max();
    max_val = std::numeric_limits<Values>::lowest();
}

template <typename Values, typename Weight>
std::vector<std::pair<Values, Weight>> icp_tdigest<Values, Weight>::get() const
{
    std::vector<std::pair<Values, Weight>> to_return;

    std::transform((*active).values.begin(), (*active).values.end(), std::back_inserter(to_return),
                   [](const centroid_t &val) {
                       return (std::make_pair(val.mean, val.weight));});

    return (to_return);
}

/**
 * When taken together the following functions comprise the scaling function for
 * t-digests.
 *
 * This is a translation of "k_2" from the tdunning reference implementation.
 */
static double Z(double compression, double n)
{
    return 4 * log(n / compression) + 24;
}

static double normalizer_fn(double compression, double n)
{
    return compression / Z(compression, n);
}

static double k(double q, double normalizer)
{
    const double q_min = 1e-15;
    const double q_max = 1 - q_min;
    if (q < q_min) {
        return 2 * k(q_min, normalizer);
    } else if (q > q_max) {
        return 2 * k(q_max, normalizer);
    }

    return log(q / (1 - q)) * normalizer;
}

static double q(double k, double normalizer)
{
    double w = exp(k / normalizer);
    return w / (1 + w);
}

template <typename Values, typename Weight>
void icp_tdigest<Values, Weight>::merge()
{
    tdigest_impl &inactive = (&one == active.load()) ? two : one;

    if (run_forward) {
        std::sort(buffer.values.begin(), buffer.values.end(),
                  std::less<centroid_t>());
    } else {
        std::sort(buffer.values.begin(), buffer.values.end(),
                  std::greater<centroid_t>());
    }

    // Update max and min values for this t-digest.
    max_val =
      std::max(max_val, run_forward ? (buffer.values.end() - 1)->mean : buffer.values.begin()->mean);
    min_val =
      std::min(min_val, run_forward ? (buffer.values.begin()->mean) : (buffer.values.end() - 1)->mean);

    const Weight new_total_weight = buffer.total_weight + (*active).total_weight;
    const double normalizer       = normalizer_fn(inactive.values.capacity(), new_total_weight);
    double k1                     = k(0, normalizer);
    double next_q_limit_weight    = new_total_weight * q(k1 + 1, normalizer);
    // printf("normalizer: %f, new_total_weight: %llu, compression: %lu, q: %f\n",
    // normalizer, new_total_weight, inactive.values.capacity(), que);
    // printf("first value of next_q_limit_weight: %f, k1: %f\n",
    // next_q_limit_weight, k1); printf("normalizer: %f\n", normalizer);

    double weight_so_far = 0;
    double weight_to_add = buffer.values.begin()->weight;
    double mean_to_add   = buffer.values.begin()->mean;

    auto compress_fn = [&inactive, new_total_weight, &k1, normalizer, &next_q_limit_weight,
                        &weight_so_far, &weight_to_add,
                        &mean_to_add](const centroid_t &current) {
        if ((weight_so_far + weight_to_add + current.weight) <= next_q_limit_weight) {
            // std::cout << "cache mean: " << src_ptr->mean << " and weight: "
            // << src_ptr->weight << std::endl;
            weight_to_add += current.weight;
            assert(weight_to_add);
            mean_to_add =
              mean_to_add + (current.mean - mean_to_add) * current.weight / weight_to_add;

        } else {
            // std::cout << "adding new centroid with mean: " << mean_to_add
            // << " and weight:" << weight_to_add << std::endl;
            weight_so_far += weight_to_add;

            double new_q =
              static_cast<double>(weight_so_far) / static_cast<double>(new_total_weight);
            k1 = k(new_q, normalizer);
            // que = q(k1 + 1, normalizer);
            next_q_limit_weight = new_total_weight * q(k1 + 1, normalizer);
            // printf("weight_so_far: %llu, next_q_limit_weight: %f, k1: %f,
            // q: %f\n", weight_so_far, next_q_limit_weight, k1, que);
            // std::cout << "after calcuating new q_limit. weight_so_far: " <<
            // weight_so_far
            //           << " next_q_limit_weight: " << next_q_limit_weight
            //           << " k1: " << k1 << " q: " << que << " new_q: " <<
            //           new_q
            //           << std::endl;

            if constexpr (std::is_integral<Values>::value) {
                mean_to_add = std::round(mean_to_add);
            }
            inactive.insert(mean_to_add, weight_to_add);
            mean_to_add   = current.mean;
            weight_to_add = current.weight;
        }
    };

    std::for_each(buffer.values.begin() + 1, buffer.values.end(), compress_fn);

    if (weight_to_add != 0) { inactive.insert(mean_to_add, weight_to_add); }

    if (!run_forward) { std::sort(inactive.values.begin(), inactive.values.end()); }
    run_forward = !run_forward;

    buffer.reset();

    // Seed buffer with the current t-digest in preparation for the next
    // merge.
    buffer.values.assign(inactive.values.begin(), inactive.values.end());

    auto new_inactive = active.exchange(&inactive);
    new_inactive->reset();
}

// FIXME - change to return std::optional<double>
// p is percentile in the range [0.0, 100.0]
template <typename Values, typename Weight>
double icp_tdigest<Values, Weight>::quantile(double p) const
{
    if (p < 0 || p > 100) {
        return -1;  // FIXME - return optional::error.
    }

    if ((*active).values.empty()) {
        // FIXME - return std::optional error.
        return -2222;
    } else if ((*active).values.size() == 1) {
        return ((*active).values.begin()->mean);
    }

    const Weight index = (static_cast<double>(p) / 100) * (*active).total_weight;

    if (index < 1) { return (min_val); }

    // For smaller quantiles, interpolate between minimum value and the first
    // centroid.
    auto first = (*active).values.begin();
    if (first->weight > 1 && index < (first->weight / 2)) {
        return (min_val + (index - 1) / (first->weight / 2 - 1) * (first->mean - min_val));
    }

    if (index > (*active).total_weight - 1) { return (max_val); }

    // For larger quantiles, interpolate between maximum value and the last
    // centroid.
    auto last = (*active).values.end() - 1;
    if (last->weight > 1 && (*active).total_weight - index <= last->weight / 2) {
        return (max_val
                - ((*active).total_weight - index - 1) / (last->weight / 2 - 1) * (max_val - last->mean));
    }

    Weight weight_so_far = (*active).values.begin()->weight / 2;
    double quantile      = 0;
    // printf("index(%llu)\n", index);
    auto quantile_fn = [index, &weight_so_far,
                        &quantile](const centroid_t &left,
                                   const centroid_t &right) {
        Weight delta_weight = (left.weight + right.weight) / 2;
        if (weight_so_far + delta_weight > index) {
            Weight lower = index - weight_so_far;
            Weight upper = weight_so_far + delta_weight - index;

            quantile = (left.mean * upper + right.mean * lower) / (lower + upper);
            // printf("lower(%llu), upper(%llu), left.mean(%d), right.mean(%d)\n",
            // lower, upper, left.mean, right.mean);
            // std::cout << "lower: " << lower << " upper: " << upper << "
            // left.mean: " << left.mean << " right.mean: " << right.mean <<
            // std::endl; printf("num(%f), den(%f)\n", num, den);
            return true;
        }

        weight_so_far += delta_weight;
        return false;
    };

    auto it = std::adjacent_find((*active).values.begin(), (*active).values.end(), quantile_fn);

    if (it == (*active).values.end()) {
        // probbaly an error.
        // FIXME - return std::optional::error
    }

    return quantile;
}

// FIXME - make it return std::optional?
template <typename Values, typename Weight>
double icp_tdigest<Values, Weight>::cumulative_distribution(Values x) const
{
    // Is the t-digest small?
    if ((*active).values.empty()) {
        // FIXME - return optional no value here.
        return -111111;
    }
    if ((*active).values.size() == 1) {
        if (x < min_val) { return (0); }
        if (x > max_val) { return (1); }
        if (x - min_val <= (max_val - min_val)) { return (0.5); }
        return ((x - min_val) / (max_val - min_val));
    }

    // Is x at one of the extremes?
    if (x < (*active).values.begin()->mean) {
        auto first = (*active).values.begin();
        if (first->mean - min_val > 0) {
            return ((1 + (x - min_val) / (first->mean - min_val) * (first->weight / 2 - 1))
                    / (*active).total_weight);
        }
        return (0);
    }
    if (x > ((*active).values.end() - 1)->mean) {
        auto last = (*active).values.end() - 1;
        if (max_val - last->mean > 0) {
            return (1
                    - ((1 + (max_val - x) / (max_val - last->mean) * (last->weight / 2 - 1))
                       / (*active).total_weight));
        }
        return (1);
    }

    Weight weight_so_far = 0;
    double cdf           = 0;
    auto cdf_fn          = [x, &weight_so_far, &cdf,
                   total_weight = (*active).total_weight](const centroid_t &left,
                                                        const centroid_t &right) {
        assert(total_weight);
        if (left.mean <= x && x < right.mean) {
            // x is bracketed between left and right.
            // printf("left.mean(%hu), x(%hu), right.mean(%hu)\n", left.mean, x,
            // right.mean);

            Weight delta_weight = (right.weight + left.weight) / static_cast<Weight>(2);
            double base         = weight_so_far + (left.weight / static_cast<Values>(2));

            //FIXME - rename val2.
            double val2 = base + delta_weight * (x - left.mean) / (right.mean - left.mean);
            // printf("delta_weight(%llu), base(%f), val2(%f)\n",
            // delta_weight, base, val2);
            cdf = val2 / total_weight;

            return true;
        }

        weight_so_far += left.weight;
        return false;
    };

    auto it = std::adjacent_find((*active).values.begin(), (*active).values.end(), cdf_fn);

    // Did we fail to find a pair of bracketing centroids?
    if (it == (*active).values.end()) {
        // Might be between max_val and the last centroid.
        if (x == ((*active).values.end() - 1)->mean) {
            return ((1 - 0.5 / (*active).total_weight));
        }
    }

    return (cdf);
}

} // namespace icp::stats
