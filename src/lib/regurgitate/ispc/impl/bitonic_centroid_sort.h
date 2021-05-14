#ifndef _REGURGITATE_IMPL_BITONIC_CENTROID_SORT_H_
#define _REGURGITATE_IMPL_BITONIC_CENTROID_SORT_H_

#include "bitonic_centroid_sort_x4.h"
#include "bitonic_centroid_sort_x8.h"

static const uniform int chunk_size = programCount * programCount;

static inline unmasked void
do_bitonic_centroid_merge(uniform key_type in_keys[],
                          uniform val_type in_vals[],
                          uniform int32 pivot,
                          uniform int32 length,
                          uniform key_type out_keys[],
                          uniform val_type out_vals[])
{
    assert(pivot < length);

    /*
     * We use two arrays of centroids to process the data
     * on the left and right sides of the pivot point.
     */
    centroid a[programCount], b[programCount];

    /* Load the `a` centroid */
    uniform int32 cursor_l = 0, cursor_r = pivot;
    uniform int32 len_l = min(chunk_size, pivot), len_r = 0;
    load_chunk(a, in_keys, in_vals, len_l);
    cursor_l += len_l;

    uniform int cursor = 0;
    for (; cursor < length; cursor += chunk_size) {
        /* Now, load the `b` centroid */
        if (cursor_l < pivot) {
            /* left values are still available */
            if (cursor_r < length) {
                /* right values are still available */
                if (in_keys[cursor_l] < in_keys[cursor_r]) {
                    len_l = min(chunk_size, pivot - cursor_l);
                    load_chunk(
                        b, in_keys + cursor_l, in_vals + cursor_l, len_l);
                    cursor_l += len_l;
                } else {
                    len_r = min(chunk_size, length - cursor_r);
                    load_chunk(
                        b, in_keys + cursor_r, in_vals + cursor_r, len_r);
                    cursor_r += len_r;
                }
            } else {
                /* only have left values */
                len_l = min(chunk_size, pivot - cursor_l);
                load_chunk(b, in_keys + cursor_l, in_vals + cursor_l, len_l);
                cursor_l += len_l;
            }
        } else {
            /* only have right values */
            len_r = min(chunk_size, length - cursor_r);
            load_chunk(b, in_keys + cursor_r, in_vals + cursor_r, len_r);
            cursor_r += len_r;
        }

        /* Reverse the b values so we can merge them */
        for (uniform int i = 0; i < programCount / 2; i++) {
            const uniform int j = programCount - 1 - i;
            centroid tmp = b[i];
            b[i] = shuffle(b[j], reverse_mask);
            b[j] = shuffle(tmp, reverse_mask);
        }

        /* Merge a and b together */
        do_centroid_chunk_merge(a, b);

        /*
         * Write the `a` centroid out. It contains the minimum values
         * from `a` and `b`.
         */
        store_chunk(a,
                    out_keys + cursor,
                    out_vals + cursor,
                    min(chunk_size, length - cursor));

        /* The `b` centroid becomes our new `a` */
        for (uniform int i = 0; i < programCount; i++) { a[i] = b[i]; }
    }
}

static inline unmasked void
bitonic_centroid_merge(uniform key_type in_means[],
                       uniform val_type in_weights[],
                       uniform key_type scratch_m[],
                       uniform val_type scratch_w[],
                       uniform int32 length)
{
    uniform key_type* uniform src_means = in_means;
    uniform val_type* uniform src_weights = in_weights;
    uniform key_type* uniform dst_means = scratch_m;
    uniform val_type* uniform dst_weights = scratch_w;

    /*
     * For each stride size, merge the 2 adjacent stride length sections
     * together
     */
    for (uniform int stride = chunk_size; stride < length; stride *= 2) {
        uniform int offset = 0;
        for (; offset + stride < length; offset += stride * 2) {
            do_bitonic_centroid_merge(src_means + offset,
                                      src_weights + offset,
                                      min(stride, length - offset),
                                      min(stride * 2, length - offset),
                                      dst_means + offset,
                                      dst_weights + offset);
        }

        /* Whatever we have left should already be sorted; copy it over */
        foreach(i = offset... length)
        {
            dst_means[i] = src_means[i];
            dst_weights[i] = src_weights[i];
        }

        uniform key_type* uniform tmp_means = src_means;
        uniform val_type* uniform tmp_weights = src_weights;
        src_means = dst_means;
        src_weights = dst_weights;
        dst_means = tmp_means;
        dst_weights = tmp_weights;
    }

    if (src_means != in_means) {
        memcpy(in_means, src_means, length * sizeof(uniform key_type));
        memcpy(in_weights, src_weights, length * sizeof(uniform val_type));
    }
}

static inline unmasked void bitonic_centroid_sort(uniform key_type means[],
                                                  uniform val_type weights[],
                                                  uniform int32 length,
                                                  uniform key_type scratch_m[],
                                                  uniform val_type scratch_w[])
{
    for (uniform int i = 0; i < length; i += chunk_size) {
        do_centroid_chunk_sort(
            means + i, weights + i, min(chunk_size, length - i));
    }

    bitonic_centroid_merge(means, weights, scratch_m, scratch_w, length);
}

#endif /* _REGURGITATE_IMPL_BITONIC_CENTROID_SORT_H_ */
