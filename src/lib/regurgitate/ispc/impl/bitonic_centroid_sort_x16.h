#ifndef _REGURGITATE_IMPL_BITONIC_CENTROID_SORT_X16_H_
#define _REGURGITATE_IMPL_BITONIC_CENTROID_SORT_X16_H_

#include "bitonic_centroid_sort_common.h"

#if TARGET_WIDTH == 16

static const int reverse_mask = {
    15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};

/* Merge 16x16 -> 32x8 */
static inline unmasked void do_centroid_merge_16x16(struct centroid& a,
                                                    struct centroid& b,
                                                    struct centroid& c,
                                                    struct centroid& d,
                                                    struct centroid& e,
                                                    struct centroid& f,
                                                    struct centroid& g,
                                                    struct centroid& h,
                                                    struct centroid& i,
                                                    struct centroid& j,
                                                    struct centroid& k,
                                                    struct centroid& l,
                                                    struct centroid& m,
                                                    struct centroid& n,
                                                    struct centroid& o,
                                                    struct centroid& p)
{
    const int masks[] = {
        {0, 1, 2, 3, 4, 5, 6, 7, 16, 17, 18, 19, 20, 21, 22, 23},
        {8, 9, 10, 11, 12, 13, 14, 15, 24, 25, 26, 27, 28, 29, 30, 31},
        {0, 1, 2, 3, 8, 9, 10, 11, 16, 17, 18, 19, 24, 25, 26, 27},
        {4, 5, 6, 7, 12, 13, 14, 15, 20, 21, 22, 23, 28, 29, 30, 31},
        {0, 1, 4, 5, 8, 9, 12, 13, 16, 17, 20, 21, 24, 25, 28, 29},
        {2, 3, 6, 7, 10, 11, 14, 15, 18, 19, 22, 23, 26, 27, 30, 31},
        {0, 2, 4, 6, 8, 10, 12, 14, 16, 18, 20, 22, 24, 26, 28, 30},
        {1, 3, 5, 7, 9, 11, 13, 15, 17, 19, 21, 23, 25, 27, 29, 31},
        {0, 16, 8, 24, 4, 20, 12, 28, 2, 18, 10, 26, 6, 22, 14, 30},
        {1, 17, 9, 25, 5, 21, 13, 29, 3, 19, 11, 27, 7, 23, 15, 31}};

    do_minmax(a, b);
    do_minmax(c, d);
    do_minmax(e, f);
    do_minmax(g, h);
    do_minmax(i, j);
    do_minmax(k, l);
    do_minmax(m, n);
    do_minmax(o, p);

    struct centroid a1 = shuffle(a, b, masks[0]);
    struct centroid b1 = shuffle(a, b, masks[1]);
    struct centroid c1 = shuffle(c, d, masks[0]);
    struct centroid d1 = shuffle(c, d, masks[1]);
    struct centroid e1 = shuffle(e, f, masks[0]);
    struct centroid f1 = shuffle(e, f, masks[1]);
    struct centroid g1 = shuffle(g, h, masks[0]);
    struct centroid h1 = shuffle(g, h, masks[1]);
    struct centroid i1 = shuffle(i, j, masks[0]);
    struct centroid j1 = shuffle(i, j, masks[1]);
    struct centroid k1 = shuffle(k, l, masks[0]);
    struct centroid l1 = shuffle(k, l, masks[1]);
    struct centroid m1 = shuffle(m, n, masks[0]);
    struct centroid n1 = shuffle(m, n, masks[1]);
    struct centroid o1 = shuffle(o, p, masks[0]);
    struct centroid p1 = shuffle(o, p, masks[1]);

    do_minmax(a1, b1);
    do_minmax(c1, d1);
    do_minmax(e1, f1);
    do_minmax(g1, h1);
    do_minmax(i1, j1);
    do_minmax(k1, l1);
    do_minmax(m1, n1);
    do_minmax(o1, p1);

    struct centroid a2 = shuffle(a1, b1, masks[2]);
    struct centroid b2 = shuffle(a1, b1, masks[3]);
    struct centroid c2 = shuffle(c1, d1, masks[2]);
    struct centroid d2 = shuffle(c1, d1, masks[3]);
    struct centroid e2 = shuffle(e1, f1, masks[2]);
    struct centroid f2 = shuffle(e1, f1, masks[3]);
    struct centroid g2 = shuffle(g1, h1, masks[2]);
    struct centroid h2 = shuffle(g1, h1, masks[3]);
    struct centroid i2 = shuffle(i1, j1, masks[2]);
    struct centroid j2 = shuffle(i1, j1, masks[3]);
    struct centroid k2 = shuffle(k1, l1, masks[2]);
    struct centroid l2 = shuffle(k1, l1, masks[3]);
    struct centroid m2 = shuffle(m1, n1, masks[2]);
    struct centroid n2 = shuffle(m1, n1, masks[3]);
    struct centroid o2 = shuffle(o1, p1, masks[2]);
    struct centroid p2 = shuffle(o1, p1, masks[3]);

    do_minmax(a2, b2);
    do_minmax(c2, d2);
    do_minmax(e2, f2);
    do_minmax(g2, h2);
    do_minmax(i2, j2);
    do_minmax(k2, l2);
    do_minmax(m2, n2);
    do_minmax(o2, p2);

    struct centroid a3 = shuffle(a2, b2, masks[4]);
    struct centroid b3 = shuffle(a2, b2, masks[5]);
    struct centroid c3 = shuffle(c2, d2, masks[4]);
    struct centroid d3 = shuffle(c2, d2, masks[5]);
    struct centroid e3 = shuffle(e2, f2, masks[4]);
    struct centroid f3 = shuffle(e2, f2, masks[5]);
    struct centroid g3 = shuffle(g2, h2, masks[4]);
    struct centroid h3 = shuffle(g2, h2, masks[5]);
    struct centroid i3 = shuffle(i2, j2, masks[4]);
    struct centroid j3 = shuffle(i2, j2, masks[5]);
    struct centroid k3 = shuffle(k2, l2, masks[4]);
    struct centroid l3 = shuffle(k2, l2, masks[5]);
    struct centroid m3 = shuffle(m2, n2, masks[4]);
    struct centroid n3 = shuffle(m2, n2, masks[5]);
    struct centroid o3 = shuffle(o2, p2, masks[4]);
    struct centroid p3 = shuffle(o2, p2, masks[5]);

    do_minmax(a3, b3);
    do_minmax(c3, d3);
    do_minmax(e3, f3);
    do_minmax(g3, h3);
    do_minmax(i3, j3);
    do_minmax(k3, l3);
    do_minmax(m3, n3);
    do_minmax(o3, p3);

    struct centroid a4 = shuffle(a3, b3, masks[6]);
    struct centroid b4 = shuffle(a3, b3, masks[7]);
    struct centroid c4 = shuffle(c3, d3, masks[6]);
    struct centroid d4 = shuffle(c3, d3, masks[7]);
    struct centroid e4 = shuffle(e3, f3, masks[6]);
    struct centroid f4 = shuffle(e3, f3, masks[7]);
    struct centroid g4 = shuffle(g3, h3, masks[6]);
    struct centroid h4 = shuffle(g3, h3, masks[7]);
    struct centroid i4 = shuffle(i3, j3, masks[6]);
    struct centroid j4 = shuffle(i3, j3, masks[7]);
    struct centroid k4 = shuffle(k3, l3, masks[6]);
    struct centroid l4 = shuffle(k3, l3, masks[7]);
    struct centroid m4 = shuffle(m3, n3, masks[6]);
    struct centroid n4 = shuffle(m3, n3, masks[7]);
    struct centroid o4 = shuffle(o3, p3, masks[6]);
    struct centroid p4 = shuffle(o3, p3, masks[7]);

    do_minmax(a4, b4);
    do_minmax(c4, d4);
    do_minmax(e4, f4);
    do_minmax(g4, h4);
    do_minmax(i4, j4);
    do_minmax(k4, l4);
    do_minmax(m4, n4);
    do_minmax(o4, p4);

    a = shuffle(a4, b4, masks[8]);
    b = shuffle(a4, b4, masks[9]);
    c = shuffle(c4, d4, masks[8]);
    d = shuffle(c4, d4, masks[9]);
    e = shuffle(e4, f4, masks[8]);
    f = shuffle(e4, f4, masks[9]);
    g = shuffle(g4, h4, masks[8]);
    h = shuffle(g4, h4, masks[9]);
    i = shuffle(i4, j4, masks[8]);
    j = shuffle(i4, j4, masks[9]);
    k = shuffle(k4, l4, masks[8]);
    l = shuffle(k4, l4, masks[9]);
    m = shuffle(m4, n4, masks[8]);
    n = shuffle(m4, n4, masks[9]);
    o = shuffle(o4, p4, masks[8]);
    p = shuffle(o4, p4, masks[9]);
}

static inline unmasked void do_centroid_merge_32x8(struct centroid& a1,
                                                   struct centroid& a2,
                                                   struct centroid& b1,
                                                   struct centroid& b2,
                                                   struct centroid& c1,
                                                   struct centroid& c2,
                                                   struct centroid& d1,
                                                   struct centroid& d2,
                                                   struct centroid& e1,
                                                   struct centroid& e2,
                                                   struct centroid& f1,
                                                   struct centroid& f2,
                                                   struct centroid& g1,
                                                   struct centroid& g2,
                                                   struct centroid& h1,
                                                   struct centroid& h2)
{
    do_minmax(a1, b1);
    do_minmax(a2, b2);
    do_minmax(c1, d1);
    do_minmax(c2, d2);
    do_minmax(e1, f1);
    do_minmax(e2, f2);
    do_minmax(g1, h1);
    do_minmax(g2, h2);

    do_centroid_merge_16x16(
        a1, a2, b1, b2, c1, c2, d1, d2, e1, e2, f1, f2, g1, g2, h1, h2);
}

static inline unmasked void do_centroid_merge_64x4(struct centroid& a1,
                                                   struct centroid& a2,
                                                   struct centroid& a3,
                                                   struct centroid& a4,
                                                   struct centroid& b1,
                                                   struct centroid& b2,
                                                   struct centroid& b3,
                                                   struct centroid& b4,
                                                   struct centroid& c1,
                                                   struct centroid& c2,
                                                   struct centroid& c3,
                                                   struct centroid& c4,
                                                   struct centroid& d1,
                                                   struct centroid& d2,
                                                   struct centroid& d3,
                                                   struct centroid& d4)
{
    do_minmax(a1, b1);
    do_minmax(a2, b2);
    do_minmax(a3, b3);
    do_minmax(a4, b4);
    do_minmax(c1, d1);
    do_minmax(c2, d2);
    do_minmax(c3, d3);
    do_minmax(c4, d4);

    do_centroid_merge_32x8(
        a1, a2, a3, a4, b1, b2, b3, b4, c1, c2, c3, c4, d1, d2, d3, d4);
}

static inline unmasked void do_centroid_merge_128x2(struct centroid& a1,
                                                    struct centroid& a2,
                                                    struct centroid& a3,
                                                    struct centroid& a4,
                                                    struct centroid& a5,
                                                    struct centroid& a6,
                                                    struct centroid& a7,
                                                    struct centroid& a8,
                                                    struct centroid& b1,
                                                    struct centroid& b2,
                                                    struct centroid& b3,
                                                    struct centroid& b4,
                                                    struct centroid& b5,
                                                    struct centroid& b6,
                                                    struct centroid& b7,
                                                    struct centroid& b8)
{
    do_minmax(a1, b1);
    do_minmax(a2, b2);
    do_minmax(a3, b3);
    do_minmax(a4, b4);
    do_minmax(a5, b5);
    do_minmax(a6, b6);
    do_minmax(a7, b7);
    do_minmax(a8, b8);

    do_centroid_merge_64x4(
        a1, a2, a3, a4, a5, a6, a7, a8, b1, b2, b3, b4, b5, b6, b7, b8);
}

static inline unmasked void do_transpose_16x16(struct centroid& a,
                                               struct centroid& b,
                                               struct centroid& c,
                                               struct centroid& d,
                                               struct centroid& e,
                                               struct centroid& f,
                                               struct centroid& g,
                                               struct centroid& h,
                                               struct centroid& i,
                                               struct centroid& j,
                                               struct centroid& k,
                                               struct centroid& l,
                                               struct centroid& m,
                                               struct centroid& n,
                                               struct centroid& o,
                                               struct centroid& p)
{
    const int masks[] = {
        {0, 16, 1, 17, 2, 18, 3, 19, 4, 20, 5, 21, 6, 22, 7, 23},
        {8, 24, 9, 25, 10, 26, 11, 27, 12, 28, 13, 29, 14, 30, 15, 31},
        {0, 1, 16, 17, 2, 3, 18, 19, 4, 5, 20, 21, 6, 7, 22, 23},
        {8, 9, 24, 25, 10, 11, 26, 27, 12, 13, 28, 29, 14, 15, 30, 31},
        {0, 1, 2, 3, 16, 17, 18, 19, 4, 5, 6, 7, 20, 21, 22, 23},
        {8, 9, 10, 11, 24, 25, 26, 27, 12, 13, 14, 15, 28, 29, 30, 31},
        {0, 1, 2, 3, 4, 5, 6, 7, 16, 17, 18, 19, 20, 21, 22, 23},
        {8, 9, 10, 11, 12, 13, 14, 15, 24, 25, 26, 27, 28, 29, 30, 31}};

    centroid a1 = shuffle(a, b, masks[0]); // a0, b0, a1, b1, ..., a7, b7
    centroid c1 = shuffle(c, d, masks[0]); // c0, d0, c1, d1, ..., c7, d7
    centroid e1 = shuffle(e, f, masks[0]); // e0, f0, e1, f1, ..., e7, f7
    centroid g1 = shuffle(g, h, masks[0]); // g0, h0, g1, h1, ..., g7, h7
    centroid i1 = shuffle(i, j, masks[0]); // i0, j0, i1, j1, ..., i7, j7
    centroid k1 = shuffle(k, l, masks[0]); // k0, l0, k1, l1, ..., k7, l7
    centroid m1 = shuffle(m, n, masks[0]); // m0, n0, m1, n1, ..., m7, n7
    centroid o1 = shuffle(o, p, masks[0]); // o0, p0, o1, p1, ..., o7, p7

    centroid b1 = shuffle(a, b, masks[1]); // a8, b8, a9, b9, ..., a15, b15
    centroid d1 = shuffle(c, d, masks[1]); // c8, d8, c9, d9, ..., c15, d15
    centroid f1 = shuffle(e, f, masks[1]); // e8, f8, e9, f9, ..., e15, f15
    centroid h1 = shuffle(g, h, masks[1]); // g8, h8, g9, h9, ..., g15, h15
    centroid j1 = shuffle(i, j, masks[1]); // i8, j8, i9, j9, ..., i15, j15
    centroid l1 = shuffle(k, l, masks[1]); // k8, l8, k9, l9, ..., k15, l15
    centroid n1 = shuffle(m, n, masks[1]); // m8, n8, m9, n9, ..., m15, n15
    centroid p1 = shuffle(o, p, masks[1]); // o8, p8, o9, p9, ..., o15, p15

    centroid a2 =
        shuffle(a1, c1, masks[2]); // a0 ... d0, a1 ... d1, a2 ... d2, a3 ... e3
    centroid c2 = shuffle(e1, g1, masks[2]); // e0 ... h0
    centroid e2 = shuffle(i1, k1, masks[2]); // i0 ... j0
    centroid g2 = shuffle(m1, o1, masks[2]); // m0 ... p0
    centroid i2 = shuffle(
        b1, d1, masks[2]); // a8 ... d8, a9 ... d9, a10 ... d10, a11 ... d11
    centroid k2 = shuffle(f1, h1, masks[2]); // e8 ... d8
    centroid m2 = shuffle(j1, l1, masks[2]); // i8 ... j8
    centroid o2 = shuffle(n1, p1, masks[2]); // m8 ... p8

    centroid b2 =
        shuffle(a1, c1, masks[3]); // a4 ... d4, a5 ... d5, a6 ... d6, a7 ... d7
    centroid d2 = shuffle(e1, g1, masks[3]); // e4 ... h4
    centroid f2 = shuffle(i1, k1, masks[3]); // i4 ... l4
    centroid h2 = shuffle(m1, o1, masks[3]); // m4 ... p4
    centroid j2 = shuffle(
        b1, d1, masks[3]); // a12 ... d12, a13 ... d13, a14 ... d14, a15 ... d15
    centroid l2 = shuffle(f1, h1, masks[3]); // e12 ... h12
    centroid n2 = shuffle(j1, l1, masks[3]); // i12 ... l12
    centroid p2 = shuffle(n1, p1, masks[3]); // m12 ... p12

    centroid a3 = shuffle(a2, c2, masks[4]); // a0 ... h0, a1 ... h1
    centroid c3 = shuffle(e2, g2, masks[4]); // i0 ... p0, i1 ... p1
    centroid e3 = shuffle(i2, k2, masks[4]); // a8 ... h8, a9 ... h9
    centroid g3 = shuffle(m2, o2, masks[4]); // i8 ... p8, i9 ... p9
    centroid i3 = shuffle(b2, d2, masks[4]); // a4 ... h4, a5 ... h5
    centroid k3 = shuffle(f2, h2, masks[4]); // i4 ... p4, i5 ... p5
    centroid m3 = shuffle(j2, l2, masks[4]); // a12 ... h12, a13 ... h13
    centroid o3 = shuffle(n2, p2, masks[4]); // i12 ... p12, a13 ... p13

    centroid b3 = shuffle(a2, c2, masks[5]); // a2 ... h2, a3 ... h3
    centroid d3 = shuffle(e2, g2, masks[5]); // i2 ... p2, i3 ... p3
    centroid f3 = shuffle(i2, k2, masks[5]); // a10 ... h10, a11 ... h11
    centroid h3 = shuffle(m2, o2, masks[5]); // i10 ... p10, i11 ... p11
    centroid j3 = shuffle(b2, d2, masks[5]); // a6 ... h6, a7 ... h7
    centroid l3 = shuffle(f2, h2, masks[5]); // i6 ... p6, i7 ... p7
    centroid n3 = shuffle(j2, l2, masks[5]); // a14 ... h14, a15 ... i15
    centroid p3 = shuffle(n2, p2, masks[5]); // i14 ... p14, i15 ... p15

    a = shuffle(a3, c3, masks[6]);
    b = shuffle(a3, c3, masks[7]);
    c = shuffle(b3, d3, masks[6]);
    d = shuffle(b3, d3, masks[7]);
    e = shuffle(i3, k3, masks[6]);
    f = shuffle(i3, k3, masks[7]);
    g = shuffle(j3, l3, masks[6]);
    h = shuffle(j3, l3, masks[7]);
    i = shuffle(e3, g3, masks[6]);
    j = shuffle(e3, g3, masks[7]);
    k = shuffle(f3, h3, masks[6]);
    l = shuffle(f3, h3, masks[7]);
    m = shuffle(m3, o3, masks[6]);
    n = shuffle(m3, o3, masks[7]);
    o = shuffle(n3, p3, masks[6]);
    p = shuffle(n3, p3, masks[7]);
}

static inline unmasked void do_centroid_chunk_sort(uniform key_type keys[],
                                                   uniform val_type vals[],
                                                   uniform uint32 length)
{
    assert(length <= 256);

    /* Load registers */
    centroid c[16];

    load_chunk(c, keys, vals, length);

    /* Sort 16 x 16 array of values */
    do_minmax(c[0], c[13]);
    do_minmax(c[1], c[12]);
    do_minmax(c[2], c[15]);
    do_minmax(c[3], c[14]);
    do_minmax(c[4], c[8]);
    do_minmax(c[5], c[6]);
    do_minmax(c[7], c[11]);
    do_minmax(c[9], c[10]);

    do_minmax(c[0], c[5]);
    do_minmax(c[1], c[7]);
    do_minmax(c[2], c[9]);
    do_minmax(c[3], c[4]);
    do_minmax(c[6], c[13]);
    do_minmax(c[8], c[14]);
    do_minmax(c[10], c[15]);
    do_minmax(c[11], c[12]);

    do_minmax(c[0], c[1]);
    do_minmax(c[2], c[3]);
    do_minmax(c[4], c[5]);
    do_minmax(c[6], c[8]);
    do_minmax(c[7], c[9]);
    do_minmax(c[10], c[11]);
    do_minmax(c[12], c[13]);
    do_minmax(c[14], c[15]);

    do_minmax(c[0], c[2]);
    do_minmax(c[1], c[3]);
    do_minmax(c[4], c[10]);
    do_minmax(c[5], c[11]);
    do_minmax(c[6], c[7]);
    do_minmax(c[8], c[9]);
    do_minmax(c[12], c[14]);
    do_minmax(c[13], c[15]);

    do_minmax(c[1], c[2]);
    do_minmax(c[3], c[12]);
    do_minmax(c[4], c[6]);
    do_minmax(c[5], c[7]);
    do_minmax(c[8], c[10]);
    do_minmax(c[9], c[11]);
    do_minmax(c[13], c[14]);

    do_minmax(c[1], c[4]);
    do_minmax(c[2], c[6]);
    do_minmax(c[5], c[8]);
    do_minmax(c[7], c[10]);
    do_minmax(c[9], c[13]);
    do_minmax(c[11], c[14]);

    do_minmax(c[2], c[4]);
    do_minmax(c[3], c[6]);
    do_minmax(c[9], c[12]);
    do_minmax(c[11], c[13]);

    do_minmax(c[3], c[5]);
    do_minmax(c[6], c[8]);
    do_minmax(c[7], c[9]);
    do_minmax(c[10], c[12]);

    do_minmax(c[3], c[4]);
    do_minmax(c[5], c[6]);
    do_minmax(c[7], c[8]);
    do_minmax(c[9], c[10]);
    do_minmax(c[11], c[12]);

    do_minmax(c[6], c[7]);
    do_minmax(c[8], c[9]);

    do_transpose_16x16(c[0],
                       c[1],
                       c[2],
                       c[3],
                       c[4],
                       c[5],
                       c[6],
                       c[7],
                       c[8],
                       c[9],
                       c[10],
                       c[11],
                       c[12],
                       c[13],
                       c[14],
                       c[15]);

    /* Merge our 16, 16 item lists into 8, 32 item lists */
    /* Reverse each odd block of 16 for merge */
    c[1] = shuffle(c[1], reverse_mask);
    c[3] = shuffle(c[3], reverse_mask);
    c[5] = shuffle(c[5], reverse_mask);
    c[7] = shuffle(c[7], reverse_mask);
    c[9] = shuffle(c[9], reverse_mask);
    c[11] = shuffle(c[11], reverse_mask);
    c[13] = shuffle(c[13], reverse_mask);
    c[15] = shuffle(c[15], reverse_mask);

    do_centroid_merge_16x16(c[0],
                            c[1],
                            c[2],
                            c[3],
                            c[4],
                            c[5],
                            c[6],
                            c[7],
                            c[8],
                            c[9],
                            c[10],
                            c[11],
                            c[12],
                            c[13],
                            c[14],
                            c[15]);

    /* Merge the 8, 32 items lists into 4, 64 item lists */
    /* Reverse each odd block of 32 for merge */
    centroid tmp = c[2];
    c[2] = shuffle(c[3], reverse_mask);
    c[3] = shuffle(tmp, reverse_mask);

    tmp = c[6];
    c[6] = shuffle(c[7], reverse_mask);
    c[7] = shuffle(tmp, reverse_mask);

    tmp = c[10];
    c[10] = shuffle(c[11], reverse_mask);
    c[11] = shuffle(tmp, reverse_mask);

    tmp = c[14];
    c[14] = shuffle(c[15], reverse_mask);
    c[15] = shuffle(tmp, reverse_mask);

    do_centroid_merge_32x8(c[0],
                           c[1],
                           c[2],
                           c[3],
                           c[4],
                           c[5],
                           c[6],
                           c[7],
                           c[8],
                           c[9],
                           c[10],
                           c[11],
                           c[12],
                           c[13],
                           c[14],
                           c[15]);

    /* Merge the 4, 64 item lists into 2, 128 item lists */
    /* Reverse each odd block of 64 for merge */
    tmp = c[4];
    c[4] = shuffle(c[7], reverse_mask);
    c[7] = shuffle(tmp, reverse_mask);

    tmp = c[5];
    c[5] = shuffle(c[6], reverse_mask);
    c[6] = shuffle(tmp, reverse_mask);

    tmp = c[12];
    c[12] = shuffle(c[15], reverse_mask);
    c[15] = shuffle(tmp, reverse_mask);

    tmp = c[13];
    c[13] = shuffle(c[14], reverse_mask);
    c[14] = shuffle(tmp, reverse_mask);

    do_centroid_merge_64x4(c[0],
                           c[1],
                           c[2],
                           c[3],
                           c[4],
                           c[5],
                           c[6],
                           c[7],
                           c[8],
                           c[9],
                           c[10],
                           c[11],
                           c[12],
                           c[13],
                           c[14],
                           c[15]);

    /* Finally, merge the 2, 128 item lists into a single 256 item list */
    /* Reverse the 2nd 128 item list for merge */
    tmp = c[8];
    c[8] = shuffle(c[15], reverse_mask);
    c[15] = shuffle(tmp, reverse_mask);

    tmp = c[9];
    c[9] = shuffle(c[14], reverse_mask);
    c[14] = shuffle(tmp, reverse_mask);

    tmp = c[10];
    c[10] = shuffle(c[13], reverse_mask);
    c[13] = shuffle(tmp, reverse_mask);

    tmp = c[11];
    c[11] = shuffle(c[12], reverse_mask);
    c[12] = shuffle(tmp, reverse_mask);

    do_centroid_merge_128x2(c[0],
                            c[1],
                            c[2],
                            c[3],
                            c[4],
                            c[5],
                            c[6],
                            c[7],
                            c[8],
                            c[9],
                            c[10],
                            c[11],
                            c[12],
                            c[13],
                            c[14],
                            c[15]);

    store_chunk(c, keys, vals, length);
}

static inline unmasked void do_centroid_chunk_merge(struct centroid a[16],
                                                    struct centroid b[16])
{
    do_minmax(a[0], b[0]);
    do_minmax(a[1], b[1]);
    do_minmax(a[2], b[2]);
    do_minmax(a[3], b[3]);
    do_minmax(a[4], b[4]);
    do_minmax(a[5], b[5]);
    do_minmax(a[6], b[6]);
    do_minmax(a[7], b[7]);
    do_minmax(a[8], b[8]);
    do_minmax(a[9], b[9]);
    do_minmax(a[10], b[10]);
    do_minmax(a[11], b[11]);
    do_minmax(a[12], b[12]);
    do_minmax(a[13], b[13]);
    do_minmax(a[14], b[14]);
    do_minmax(a[15], b[15]);

    do_centroid_merge_128x2(a[0],
                            a[1],
                            a[2],
                            a[3],
                            a[4],
                            a[5],
                            a[6],
                            a[7],
                            a[8],
                            a[9],
                            a[10],
                            a[11],
                            a[12],
                            a[13],
                            a[14],
                            a[15]);

    do_centroid_merge_128x2(b[0],
                            b[1],
                            b[2],
                            b[3],
                            b[4],
                            b[5],
                            b[6],
                            b[7],
                            b[8],
                            b[9],
                            b[10],
                            b[11],
                            b[12],
                            b[13],
                            b[14],
                            b[15]);
}

#endif

#endif /* _REGURGITATE_IMPL_BITONIC_CENTROID_SORT_X16_H_ */
