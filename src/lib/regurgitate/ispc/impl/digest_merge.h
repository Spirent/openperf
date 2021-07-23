#ifndef _REGURGITATE_DIGEST_MERGE_IMPL_H_
#define _REGURGITATE_DIGEST_MERGE_IMPL_H_

/*
 * Calculate next quantile limit given the current quantile.
 * Use the Folly t-digest approach that uses a quadratic distribution
 * instead of the original implementation's logarithmic one. This
 * has the benefit of being easier to map to traditional TestCenter
 * histograms as well as being faster.
 */
static inline uniform scale_type next_q(uniform scale_type q,
                                        uniform int nb_clusters)
{
    uniform scale_type k = select(q < 0.5, sqrt(q / 2), 1 - sqrt((1 - q) / 2));

    k += rcp((uniform float)nb_clusters);

    uniform scale_type w =
        select(k < 0.5, 2 * k * k, 1 - 2 * (1 - k) * (1 - k));

    return (w);
}

static inline uniform weight_type accumulate(uniform val_type values[],
                                             uniform uint32 length)
{
    weight_type sum = 0;

    foreach(i = 0 ... length) { sum += values[i]; }

    return (reduce_add(sum));
}

struct centroid
{
    sum_type sum;
    weight_type weight;
};

static inline void store_centroid(uniform const struct centroid& c,
                                  uniform key_type mean[],
                                  uniform val_type weight[],
                                  uniform int& idx)
{
    mean[idx] = c.sum / c.weight;
    weight[idx] = c.weight;
    idx++;
}

static inline uniform unsigned int32 merge(uniform key_type in_means[],
                                           uniform val_type in_weights[],
                                           uniform uint32 in_length,
                                           uniform key_type out_means[],
                                           uniform val_type out_weights[],
                                           uniform uint32 out_length)
{
    /*
     * Note: we might need to use higher resolution types for weight
     * sums, otherwise we could run into resolution problems with bucket
     * limits when merging large data sets.
     */
    const uniform weight_type total_weight = accumulate(in_weights, in_length);
    const uniform weight_type total_weight_inv = rcp(total_weight);

    uniform weight_type q_limit_weight = total_weight * next_q(0, out_length);
    uniform weight_type sum_weight = 0;
    uniform int bucket = 0, cursor = 0;

    /*
     * Sadly, summing centroid values across lanes means we need to
     * keep some state. We use two structures: one to store the
     * current k-means sum as we sum across the lanes and one to store
     * any partial sums for when we have to split a cluster.
     *
     * Note: partial is instantiated below since we can't carry partial
     * sums across lanes.
     */
    uniform struct centroid current = {0, 0};

    foreach(i = 0 ... in_length)
    {
        /*
         * Setup loop variables.  Note: since centroids store means,
         * we need to use mean * weight when adding them together.
         */
        sum_type mean_x_weight = in_means[i] * in_weights[i];
        weight_type lane_weight =
            exclusive_scan_add((weight_type)in_weights[i]) + in_weights[i];
        uniform weight_type cur_loop_weight = 0;
        uniform struct centroid partial = {0, 0};

        /* Run the do-while loop until we've consumed this much weight */
        const uniform weight_type max_loop_weight =
            extract(lane_weight, reduce_max(programIndex));

        do {
            /*
             * This value contains the total weight of each lane in
             * terms of our q limit. If this value is less than the q
             * limited weight, then it will fit in our current bucket.
             */
            const weight_type limit_weight =
                sum_weight + current.weight + lane_weight - cur_loop_weight;

            /* skip over any previously processed lanes */
            if (lane_weight < cur_loop_weight) { continue; };

            /*
             * Hopefully, all of the lanes belong to the same bucket and we can
             * just handle them all in one swell foop. :)
             */
            cif(limit_weight < q_limit_weight)
            {
                /*
                 * All of these lanes fit in the next bucket, so just
                 * calculate the sums across the lanes.
                 */
                current.sum += reduce_add(mean_x_weight) - partial.sum;
                uniform weight_type w =
                    reduce_add((weight_type)in_weights[i]) - partial.weight;
                current.weight += w;
                cur_loop_weight += w;
                partial.sum = 0;
                partial.weight = 0;
            }
            else
            {
                /*
                 * The first active lane contains a cluster with a
                 * weight >= to our limit. Pull the needed weight out
                 * and add it to our current bucket and our partial.
                 * The next loop through will need to subtract the
                 * partial value when generating the sum.
                 */
                uniform int active_lane = reduce_min(programIndex);
                uniform weight_type delta_w =
                    ceil(q_limit_weight) - sum_weight - current.weight;
                uniform sum_type delta_s =
                    extract(in_means[i], active_lane) * delta_w;
                assert(delta_w <= extract(in_weights[i], active_lane));

                current.sum += delta_s;
                current.weight += delta_w;

                partial.sum += delta_s;
                partial.weight += delta_w;

                cur_loop_weight += delta_w;

                assert(sum_weight + current.weight >= q_limit_weight);
                assert(current.weight > 0);
                assert(bucket < out_length);
                store_centroid(current, out_means, out_weights, bucket);

                /* And update the loop variables for the next go round */
                sum_weight += current.weight;
                uniform scale_type q = sum_weight * total_weight_inv;
                q_limit_weight = total_weight * next_q(q, out_length);

                /* Clear our temporary container */
                current.sum = 0;
                current.weight = 0;
            }
            assert(cur_loop_weight <= max_loop_weight);
        } while (cur_loop_weight < max_loop_weight);
    }

    /* Handle any leftover sum/weight data */
    if (current.weight) {
        assert(bucket < out_length);
        store_centroid(current, out_means, out_weights, bucket);
    }

    assert(sum_weight + current.weight == total_weight);

    return (bucket);
}

#endif /* _REGURGITATE_DIGEST_MERGE_IMPL_H_ */
