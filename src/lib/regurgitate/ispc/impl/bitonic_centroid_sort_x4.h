#ifndef _REGURGITATE_IMPL_BITONIC_CENTROID_SORT_X4_H_
#define _REGURGITATE_IMPL_BITONIC_CENTROID_SORT_X4_H_

#include "bitonic_centroid_sort_common.h"

#if TARGET_WIDTH == 4

static const int reverse_mask = {3, 2, 1, 0};

/* Merge 4x4 -> 8x2 */
static inline unmasked void do_centroid_merge_4x4(struct centroid& a,
                                                  struct centroid& b,
                                                  struct centroid& c,
                                                  struct centroid& d)
{
    const int masks[] = {{0, 1, 4, 5},
                         {2, 3, 6, 7},
                         {0, 4, 2, 6},
                         {1, 5, 3, 7},
                         {0, 4, 1, 5},
                         {2, 6, 3, 7}};

    do_minmax(a, b);
    do_minmax(c, d);

    struct centroid a1 = shuffle(a, b, masks[0]);
    struct centroid b1 = shuffle(a, b, masks[1]);
    struct centroid c1 = shuffle(c, d, masks[0]);
    struct centroid d1 = shuffle(c, d, masks[1]);

    do_minmax(a1, b1);
    do_minmax(c1, d1);

    struct centroid a2 = shuffle(a1, b1, masks[2]);
    struct centroid b2 = shuffle(a1, b1, masks[3]);
    struct centroid c2 = shuffle(c1, d1, masks[2]);
    struct centroid d2 = shuffle(c1, d1, masks[3]);

    do_minmax(a2, b2);
    do_minmax(c2, d2);

    a = shuffle(a2, b2, masks[4]);
    b = shuffle(a2, b2, masks[5]);
    c = shuffle(c2, d2, masks[4]);
    d = shuffle(c2, d2, masks[5]);
}

/* Merge 8x2 -> 16x1 */
static inline unmasked void do_centroid_merge_8x2(struct centroid& a1,
                                                  struct centroid& a2,
                                                  struct centroid& b1,
                                                  struct centroid& b2)
{
    do_minmax(a1, b1);
    do_minmax(a2, b2);

    do_centroid_merge_4x4(a1, a2, b1, b2);
}

static inline unmasked void do_transpose_4x4(struct centroid& a,
                                             struct centroid& b,
                                             struct centroid& c,
                                             struct centroid& d)
{
    const int masks[] = {
        {0, 2, 4, 6},
        {1, 3, 5, 7},
    };

    centroid a1 = shuffle(a, b, masks[0]);
    centroid b1 = shuffle(a, b, masks[1]);
    centroid c1 = shuffle(c, d, masks[0]);
    centroid d1 = shuffle(c, d, masks[1]);

    a = shuffle(a1, c1, masks[0]);
    b = shuffle(b1, d1, masks[0]);
    c = shuffle(a1, c1, masks[1]);
    d = shuffle(b1, d1, masks[1]);
}

static inline unmasked void do_centroid_chunk_sort(uniform key_type keys[],
                                                   uniform val_type vals[],
                                                   uniform uint32 length)
{
    assert(length <= 16);

    /* Load registers */
    centroid c[4];

    load_chunk(c, keys, vals, length);

    /* Sort 4 x 4 array of values */
    do_minmax(c[0], c[2]);
    do_minmax(c[1], c[3]);
    do_minmax(c[0], c[1]);
    do_minmax(c[2], c[3]);
    do_minmax(c[1], c[2]);

    do_transpose_4x4(c[0], c[1], c[2], c[3]);

    /* Reverse centroids for merge */
    c[1] = shuffle(c[1], reverse_mask);
    c[3] = shuffle(c[3], reverse_mask);

    do_centroid_merge_4x4(c[0], c[1], c[2], c[3]);

    struct centroid tmp = c[2];
    c[2] = shuffle(c[3], reverse_mask);
    c[3] = shuffle(tmp, reverse_mask);

    do_centroid_merge_8x2(c[0], c[1], c[2], c[3]);

    store_chunk(c, keys, vals, length);
}

/* Merge 16x2 -> 32x1 */
static inline unmasked void do_centroid_chunk_merge(struct centroid a[4],
                                                    struct centroid b[4])
{
    do_minmax(a[0], b[0]);
    do_minmax(a[1], b[1]);
    do_minmax(a[2], b[2]);
    do_minmax(a[3], b[3]);

    do_centroid_merge_8x2(a[0], a[1], a[2], a[3]);
    do_centroid_merge_8x2(b[0], b[1], b[2], b[3]);
}

#endif

#endif /* _REGURGITATE_IMPL_BITONIC_CENTROID_SORT_X4_H_ */
