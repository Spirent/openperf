#ifndef _REGURGITATE_IMPL_BITONIC_CENTROID_SORT_X8_H_
#define _REGURGITATE_IMPL_BITONIC_CENTROID_SORT_X8_H_

#include "bitonic_centroid_sort_common.h"

#if TARGET_WIDTH == 8

static const int reverse_mask = {7, 6, 5, 4, 3, 2, 1, 0};

/* Merge 8x8 -> 16x4 */
static inline unmasked void do_centroid_merge_8x8(struct centroid& a,
                                                  struct centroid& b,
                                                  struct centroid& c,
                                                  struct centroid& d,
                                                  struct centroid& e,
                                                  struct centroid& f,
                                                  struct centroid& g,
                                                  struct centroid& h)
{
    const int masks[] = {
        {0, 1, 2, 3, 8, 9, 10, 11},
        {4, 5, 6, 7, 12, 13, 14, 15},
        {0, 1, 4, 5, 8, 9, 12, 13},
        {2, 3, 6, 7, 10, 11, 14, 15},
        {0, 2, 4, 6, 8, 10, 12, 14},
        {1, 3, 5, 7, 9, 11, 13, 15},
        {0, 8, 4, 12, 2, 10, 6, 14},
        {1, 9, 5, 13, 3, 11, 7, 15},
    };

    do_minmax(a, b);
    do_minmax(c, d);
    do_minmax(e, f);
    do_minmax(g, h);

    struct centroid a1 = shuffle(a, b, masks[0]);
    struct centroid b1 = shuffle(a, b, masks[1]);
    struct centroid c1 = shuffle(c, d, masks[0]);
    struct centroid d1 = shuffle(c, d, masks[1]);
    struct centroid e1 = shuffle(e, f, masks[0]);
    struct centroid f1 = shuffle(e, f, masks[1]);
    struct centroid g1 = shuffle(g, h, masks[0]);
    struct centroid h1 = shuffle(g, h, masks[1]);

    do_minmax(a1, b1);
    do_minmax(c1, d1);
    do_minmax(e1, f1);
    do_minmax(g1, h1);

    struct centroid a2 = shuffle(a1, b1, masks[2]);
    struct centroid b2 = shuffle(a1, b1, masks[3]);
    struct centroid c2 = shuffle(c1, d1, masks[2]);
    struct centroid d2 = shuffle(c1, d1, masks[3]);
    struct centroid e2 = shuffle(e1, f1, masks[2]);
    struct centroid f2 = shuffle(e1, f1, masks[3]);
    struct centroid g2 = shuffle(g1, h1, masks[2]);
    struct centroid h2 = shuffle(g1, h1, masks[3]);

    do_minmax(a2, b2);
    do_minmax(c2, d2);
    do_minmax(e2, f2);
    do_minmax(g2, h2);

    struct centroid a3 = shuffle(a2, b2, masks[4]);
    struct centroid b3 = shuffle(a2, b2, masks[5]);
    struct centroid c3 = shuffle(c2, d2, masks[4]);
    struct centroid d3 = shuffle(c2, d2, masks[5]);
    struct centroid e3 = shuffle(e2, f2, masks[4]);
    struct centroid f3 = shuffle(e2, f2, masks[5]);
    struct centroid g3 = shuffle(g2, h2, masks[4]);
    struct centroid h3 = shuffle(g2, h2, masks[5]);

    do_minmax(a3, b3);
    do_minmax(c3, d3);
    do_minmax(e3, f3);
    do_minmax(g3, h3);

    a = shuffle(a3, b3, masks[6]);
    b = shuffle(a3, b3, masks[7]);
    c = shuffle(c3, d3, masks[6]);
    d = shuffle(c3, d3, masks[7]);
    e = shuffle(e3, f3, masks[6]);
    f = shuffle(e3, f3, masks[7]);
    g = shuffle(g3, h3, masks[6]);
    h = shuffle(g3, h3, masks[7]);
}

static inline unmasked void do_centroid_merge_16x4(struct centroid& a1,
                                                   struct centroid& a2,
                                                   struct centroid& b1,
                                                   struct centroid& b2,
                                                   struct centroid& c1,
                                                   struct centroid& c2,
                                                   struct centroid& d1,
                                                   struct centroid& d2)
{
    do_minmax(a1, b1);
    do_minmax(a2, b2);
    do_minmax(c1, d1);
    do_minmax(c2, d2);

    do_centroid_merge_8x8(a1, a2, b1, b2, c1, c2, d1, d2);
}

static inline unmasked void do_centroid_merge_32x2(struct centroid& a1,
                                                   struct centroid& a2,
                                                   struct centroid& a3,
                                                   struct centroid& a4,
                                                   struct centroid& b1,
                                                   struct centroid& b2,
                                                   struct centroid& b3,
                                                   struct centroid& b4)
{
    do_minmax(a1, b1);
    do_minmax(a2, b2);
    do_minmax(a3, b3);
    do_minmax(a4, b4);

    do_centroid_merge_16x4(a1, a2, a3, a4, b1, b2, b3, b4);
}

static inline unmasked void do_transpose_8x8(struct centroid& a,
                                             struct centroid& b,
                                             struct centroid& c,
                                             struct centroid& d,
                                             struct centroid& e,
                                             struct centroid& f,
                                             struct centroid& g,
                                             struct centroid& h)
{
    const int masks[] = {{0, 8, 1, 9, 2, 10, 3, 11},
                         {4, 12, 5, 13, 6, 14, 7, 15},
                         {0, 1, 8, 9, 2, 3, 10, 11},
                         {4, 5, 12, 13, 6, 7, 14, 15},
                         {0, 1, 2, 3, 8, 9, 10, 11},
                         {4, 5, 6, 7, 12, 13, 14, 15}};

    centroid a1 = shuffle(a, b, masks[0]);
    centroid c1 = shuffle(c, d, masks[0]);
    centroid e1 = shuffle(e, f, masks[0]);
    centroid g1 = shuffle(g, h, masks[0]);

    centroid b1 = shuffle(a, b, masks[1]);
    centroid d1 = shuffle(c, d, masks[1]);
    centroid f1 = shuffle(e, f, masks[1]);
    centroid h1 = shuffle(g, h, masks[1]);

    centroid a2 = shuffle(a1, c1, masks[2]);
    centroid c2 = shuffle(b1, d1, masks[2]);
    centroid e2 = shuffle(e1, g1, masks[2]);
    centroid g2 = shuffle(f1, h1, masks[2]);

    centroid b2 = shuffle(a1, c1, masks[3]);
    centroid d2 = shuffle(b1, d1, masks[3]);
    centroid f2 = shuffle(e1, g1, masks[3]);
    centroid h2 = shuffle(f1, h1, masks[3]);

    a = shuffle(a2, e2, masks[4]);
    b = shuffle(a2, e2, masks[5]);
    c = shuffle(b2, f2, masks[4]);
    d = shuffle(b2, f2, masks[5]);
    e = shuffle(c2, g2, masks[4]);
    f = shuffle(c2, g2, masks[5]);
    g = shuffle(d2, h2, masks[4]);
    h = shuffle(d2, h2, masks[5]);
}

static inline unmasked void do_centroid_chunk_sort(uniform key_type keys[],
                                                   uniform val_type vals[],
                                                   uniform uint32 length)
{
    assert(length <= 64);

    /* Load registers */
    centroid c[8];

    load_chunk(c, keys, vals, length);

    /* Sort 8 x 8 array of values */
    do_minmax(c[0], c[2]);
    do_minmax(c[1], c[3]);
    do_minmax(c[4], c[6]);
    do_minmax(c[5], c[7]);

    do_minmax(c[0], c[4]);
    do_minmax(c[1], c[5]);
    do_minmax(c[2], c[6]);
    do_minmax(c[3], c[7]);

    do_minmax(c[0], c[1]);
    do_minmax(c[2], c[3]);
    do_minmax(c[4], c[5]);
    do_minmax(c[6], c[7]);

    do_minmax(c[2], c[4]);
    do_minmax(c[3], c[5]);

    do_minmax(c[1], c[4]);
    do_minmax(c[3], c[6]);

    do_minmax(c[1], c[2]);
    do_minmax(c[3], c[4]);
    do_minmax(c[5], c[6]);

    do_transpose_8x8(c[0], c[1], c[2], c[3], c[4], c[5], c[6], c[7]);

    /* Merge our 8, 8 item lists into 4, 16 item lists */
    /* Reverse each odd block of 8 for merge */
    c[1] = shuffle(c[1], reverse_mask);
    c[3] = shuffle(c[3], reverse_mask);
    c[5] = shuffle(c[5], reverse_mask);
    c[7] = shuffle(c[7], reverse_mask);

    do_centroid_merge_8x8(c[0], c[1], c[2], c[3], c[4], c[5], c[6], c[7]);

    /* Merge the 4, 16 item lists into 2, 32 item lists */
    /* Reverse each odd block of 16 for merge */
    centroid tmp = c[2];
    c[2] = shuffle(c[3], reverse_mask);
    c[3] = shuffle(tmp, reverse_mask);

    tmp = c[6];
    c[6] = shuffle(c[7], reverse_mask);
    c[7] = shuffle(tmp, reverse_mask);

    do_centroid_merge_16x4(c[0], c[1], c[2], c[3], c[4], c[5], c[6], c[7]);

    /* Finally, merge the 2, 32 item lists into a single 64 item list */
    /* Reverse the 2nd 32 item list for merge */
    tmp = c[4];
    c[4] = shuffle(c[7], reverse_mask);
    c[7] = shuffle(tmp, reverse_mask);

    tmp = c[5];
    c[5] = shuffle(c[6], reverse_mask);
    c[6] = shuffle(tmp, reverse_mask);

    do_centroid_merge_32x2(c[0], c[1], c[2], c[3], c[4], c[5], c[6], c[7]);

    store_chunk(c, keys, vals, length);
}

static inline unmasked void do_centroid_chunk_merge(struct centroid a[8],
                                                    struct centroid b[8])
{
    do_minmax(a[0], b[0]);
    do_minmax(a[1], b[1]);
    do_minmax(a[2], b[2]);
    do_minmax(a[3], b[3]);
    do_minmax(a[4], b[4]);
    do_minmax(a[5], b[5]);
    do_minmax(a[6], b[6]);
    do_minmax(a[7], b[7]);

    do_centroid_merge_32x2(a[0], a[1], a[2], a[3], a[4], a[5], a[6], a[7]);
    do_centroid_merge_32x2(b[0], b[1], b[2], b[3], b[4], b[5], b[6], b[7]);
}

#endif

#endif /* _REGURGITATE_IMPL_BITONIC_CENTROID_SORT_X8_H_ */
