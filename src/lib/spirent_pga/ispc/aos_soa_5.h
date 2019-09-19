
/*
 * Transpose an array of n, 5 x 32 bit integers into 5, 32 x n integers.
 * This is used by the IPv4 headder checksumming functions to turn an
 * array of IPv4 headers into a format suitable for SIMD lane operations.
 * These are based on the existing aos_to_soa{3,4} functions in the ISPC
 * standard library.
 */

#if TARGET_WIDTH == 4

inline void aos_to_soa5(uniform int32 a[], // a0 b0 c0 d0 e0 ... a3 b3 c3 d3 e3
                        varying int32 * uniform v0,
                        varying int32 * uniform v1,
                        varying int32 * uniform v2,
                        varying int32 * uniform v3,
                        varying int32 * uniform v4)
{
    static const varying int p0 = { 0, 5, 2, 7 };
    static const varying int p1 = { 1, 4, 3, 6 };
    static const varying int p2 = { 0, 1, 6, 7 };
    static const varying int p3 = { 0, 6, 7, 1 };
    static const varying int p4 = { 2, 3, 4, 5 };
    static const varying int p5 = { 2, 4, 5, 3 };

    unmasked {
        int32 in0, in1, in2, in3, in4;
        packed_load_active(a,      &in0);  // a0 b0 c0 d0
        packed_load_active(a + 4,  &in1);  // e0 a1 b1 c1
        packed_load_active(a + 8,  &in2);  // d1 e1 a2 b2
        packed_load_active(a + 12, &in3);  // c2 d2 e2 a3
        packed_load_active(a + 16, &in4);  // b3 c3 d3 e3

        int32 tmp0 = shuffle(in0, in1, p0);  // a0 a1 c0 c1
        int32 tmp1 = shuffle(in1, in2, p0);  // e0 e1 b1 b2
        int32 tmp2 = shuffle(in2, in3, p0);  // d1 d2 a2 a3
        int32 tmp3 = shuffle(in3, in4, p0);  // c2 c3 e2 e3
        int32 tmp4 = shuffle(in0, in4, p1);  // b0 b3 d0 d3

        *v0 = shuffle(tmp0, tmp2, p2);  // a0 a1 a2 a3
        *v1 = shuffle(tmp4, tmp1, p3);  // b0 b1 b2 b3
        *v2 = shuffle(tmp0, tmp3, p4);  // c0 c1 c2 c3
        *v3 = shuffle(tmp4, tmp2, p5);  // d0 d1 d2 d3
        *v4 = shuffle(tmp1, tmp3, p2);  // e0 e1 e2 e3
    }
}

#elif TARGET_WIDTH == 8

inline void aos_to_soa5(uniform int32 a[], // a0 b0 c0 d0 e0 ... a7 b7 c7 d7 e7
                        varying int32 * uniform v0,
                        varying int32 * uniform v1,
                        varying int32 * uniform v2,
                        varying int32 * uniform v3,
                        varying int32 * uniform v4)
{
    static const varying int p0 = {  0,  1,  2,  3, 12, 13, 14, 15 };
    static const varying int p1 = {  0,  1, 10, 11,  4,  5, 14, 15 };
    static const varying int p2 = {  2,  3, 12, 13,  6,  7,  8,  9 };
    static const varying int p3 = {  4,  5, 14, 15,  0,  1, 10, 11 };
    static const varying int p4 = {  6,  7,  8,  9,  2,  3, 12, 13 };
    static const varying int p5 = {  0,  9,  2, 11,  4, 13,  6, 15 };
    static const varying int p6 = {  1,  8,  3, 10,  5, 12,  7, 14 };

    unmasked {
        int32 in0, in1, in2, in3, in4;
        packed_load_active(     a, &in0);  // a0 b0 c0 d0 e0 a1 b1 c1
        packed_load_active( a + 8, &in1);  // d1 e1 a2 b2 c2 d2 e2 a3
        packed_load_active(a + 16, &in2);  // b3 c3 d3 e3 a4 b4 c4 d4
        packed_load_active(a + 24, &in3);  // e4 a5 b5 c5 d5 e5 a6 b6
        packed_load_active(a + 32, &in4);  // c6 d6 e6 a7 b7 c7 d7 e7

        int32 tmp0 = shuffle(in0, in2, p0);  // a0 b0 c0 d0 a4 b4 c4 d4
        int32 tmp1 = shuffle(in3, in0, p0);  // e4 a5 b5 c5 e0 a1 b1 c1
        int32 tmp2 = shuffle(in1, in3, p0);  // d1 e1 a2 b2 d5 e5 a6 b6
        int32 tmp3 = shuffle(in4, in1, p0);  // c6 d6 e6 a7 c2 d2 e2 a3
        int32 tmp4 = shuffle(in2, in4, p0);  // b3 c3 d3 e3 b7 c7 d7 e7

        int32 v0v1 = shuffle(tmp0, tmp2, p1);  // a0 b0 a2 b2 a4 b4 a6 b6
        int32 v2v3 = shuffle(tmp0, tmp3, p2);  // c0 d0 c2 d2 c4 d4 c6 d6
        int32 v4v0 = shuffle(tmp1, tmp3, p3);  // e0 a1 e2 a3 e4 a5 e6 a7
        int32 v1v2 = shuffle(tmp1, tmp4, p4);  // b1 c1 b3 c3 b5 c5 b7 c7
        int32 v3v4 = shuffle(tmp2, tmp4, p1);  // d1 e1 d3 e3 d5 e5 d7 e7

        *v0 = shuffle(v0v1, v4v0, p5);  // a0 a1 a2 a3 a4 a5 a6 a7
        *v1 = shuffle(v0v1, v1v2, p6);  // b0 b1 b2 b3 b4 b5 b6 b7
        *v2 = shuffle(v2v3, v1v2, p5);  // c0 c1 c2 c3 c4 c5 c6 c7
        *v3 = shuffle(v2v3, v3v4, p6);  // d0 d1 d2 d3 d4 d5 d6 d7
        *v4 = shuffle(v4v0, v3v4, p5);  // e0 e1 e2 e3 e4 e5 e6 e7
    }
}

#elif TARGET_WIDTH == 16

inline void aos_to_soa5(uniform int32 a[], // a0 b0 c0 d0 e0 ... a15 b15 c15 d15 e15
                        varying int32 * uniform v0,
                        varying int32 * uniform v1,
                        varying int32 * uniform v2,
                        varying int32 * uniform v3,
                        varying int32 * uniform v4)
{
    static const varying int32 p0 = {  0,  1,  2,  3,  4,  5,  6,  7,
                                      24, 25, 26, 27, 28, 29, 30, 31 };
    static const varying int32 p1a = {  0,  1,  2,  3, 20, 21, 22, 23,
                                        8,  9, 10, 11, 28, 29, 30, 31 };
    static const varying int32 p1b = { 20, 21, 22, 23,  8,  9, 10, 11,
                                       28, 29, 30, 31,  0,  1,  2,  3 };
    static const varying int32 p1c = {  8,  9, 10, 11, 28, 29, 30, 31,
                                        0,  1,  2,  3, 20, 21, 22, 23 };
    static const varying int32 p2a = {  0,  1, 18, 19,  4,  5, 22, 23,
                                        8,  9, 26, 27, 12, 13, 30, 31 };
    static const varying int32 p2b = {  2,  3, 28, 29,  6,  7, 16, 17,
                                       10, 11, 20, 21, 14, 15, 24, 25 };
    static const varying int32 p2c = {  0,  1, 30, 31,  4,  5, 18, 19,
                                        8,  9, 22, 23, 12, 13, 26, 27 };
    static const varying int32 p2d = {  2,  3, 16, 17,  6,  7, 20, 21,
                                       10, 11, 24, 25, 14, 15, 28, 29 };
    static const varying int32 p3a = {  0, 17,  2, 19,  4, 21,  6, 23,
                                        8, 25, 10, 27, 12, 29, 14, 31 };
    static const varying int32 p3b = {  1, 16,  3, 18,  5, 20,  7, 22,
                                        9, 24, 11, 26, 13, 28, 15, 30 };

    unmasked {
        int32 in0, in1, in2, in3, in4;
        packed_load_active(     a, &in0);  //  a0  b0  c0  d0  e0  a1  b1  c1
                                           //  d1  e1  a2  b2  c2  d2  e2  a3
        packed_load_active(a + 16, &in1);  //  b3  c3  d3  e3  a4  b4  c4  d4
                                           //  e4  a5  b5  c5  d5  e5  a6  b6
        packed_load_active(a + 32, &in2);  //  c6  d6  e6  a7  b7  c7  d7  e7
                                           //  a8  b8  c8  d8  e8  a9  b9  c9
        packed_load_active(a + 48, &in3);  //  d9  e9 a10 b10 c10 d10 e10 a11
                                           // b11 c11 d11 e11 a12 b12 c12 d12
        packed_load_active(a + 64, &in4);  // e12 a13 b13 c13 d13 e13 a14 b14
                                           // c14 d14 e14 a15 b15 c15 d15 e15

        int32 tmp0 = shuffle(in0, in2, p0);  //  a0  b0  c0  d0  e0  a1  b1  c1
                                             //  a8  b8  c8  d8  e8  a9  b9  c9
        int32 tmp1 = shuffle(in3, in0, p0);  //  d9  e9 a10 b10 c10 d10 e10 a11
                                             //  d1  e1  a2  b2  c2  d2  e2  a3
        int32 tmp2 = shuffle(in1, in3, p0);  //  b3  c3  d3  e3  a4  b4  c4  d4
                                             // b11 c11 d11 e11 a12 b12 c12 d12
        int32 tmp3 = shuffle(in4, in1, p0);  // e12 a13 b13 c13 d13 e13 a14 b14
                                             //  e4  a5  b5  c5  d5  e5  a6  b6
        int32 tmp4 = shuffle(in2, in4, p0);  //  c6  d6  e6  a7  b7  c7  d7  e7
                                             // c14 d14 e14 a15 b15 c15 d15 e15

        int32 tmp5 = shuffle(tmp0, tmp2, p1a);  //  a0  b0  c0  d0  a4  b4  c4  d4
                                                //  a8  b8  c8  d8 a12 b12 c12 d12
        int32 tmp6 = shuffle(tmp3, tmp0, p1b);  //  e0  a1  b1  c1  e4  a5  b5  c5
                                                //  e8  a9  b9  c9 e12 a13 b13 c13
        int32 tmp7 = shuffle(tmp1, tmp3, p1c);  //  d1  e1  a2  b2  d5  e5  a6  b6
                                                //  d9  e9 a10 b10 d13 e13 a14 b14
        int32 tmp8 = shuffle(tmp4, tmp1, p1a);  //  c6  d6  e6  a7 c10 d10 e10 a11
                                                // c14 d14 e14 a15  c2  d2  e2  a3
        int32 tmp9 = shuffle(tmp2, tmp4, p1a);  //  b3  c3  d3  e3  b7  c7  d7  e7
                                                // b11 c11 d11 e11 b15 c15 d15 e15

        int32 v0v1 = shuffle(tmp5, tmp7, p2a);  //  a0  b0  a2  b2  a4  b4  a6  b6
                                                //  a8  b8 a10 b10 a12 b12 a14 b14
        int32 v2v3 = shuffle(tmp5, tmp8, p2b);  //  c0  d0  c2  d2  c4  d4  c6  d6
                                                //  c8  d8 c10 d10 c12 d12 c14 d14
        int32 v4v0 = shuffle(tmp6, tmp8, p2c);  //  e0  a1  e2  a3  e4  a5  e6  a7
                                                //  e8  a9 e10 a11 e12 a13 e14 a15
        int32 v1v2 = shuffle(tmp6, tmp9, p2d);  //  b1  c1  b3  c3  b5  c5  b7  c7
                                                //  b9  c9 b11 c11 b13 c13 b15 c15
        int32 v3v4 = shuffle(tmp7, tmp9, p2a);  //  d1  e1  d3  e3  d5  e5  d7  e7
                                                //  d9  e9 d11 e11 d13 e13 d15 e15

        *v0 = shuffle(v0v1, v4v0, p3a);  // a0 a1 a2 ... a15
        *v1 = shuffle(v0v1, v1v2, p3b);  // b0 b1 b2 ... b15
        *v2 = shuffle(v2v3, v1v2, p3a);  // c0 c1 c2 ... c15
        *v3 = shuffle(v2v3, v3v4, p3b);  // d0 d1 d2 ... d15
        *v4 = shuffle(v4v0, v3v4, p3a);  // e0 e1 e2 ... e15
    }
}

#endif
