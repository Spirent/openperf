/*

The MIT License (MIT)

Copyright (c) 2015 Jacques-Henri Jourdan <jourgun@gmail.com>

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

*/

#ifndef _REGURGITATE_IMPL_MATH_APPROXIMATIONS_H_
#define _REGURGITATE_IMPL_MATH_APPROXIMATIONS_H_

static const uniform int32 infinity = 0x7f800000;
static const uniform float exp_cst1_f = 2139095040.f;
static const uniform float exp_cst2_f = 0.f;

static inline uniform float exp_approx(uniform float val)
{
    uniform int32 tmp =
        clamp(12102203.1615614f * val + 1065353216.f, exp_cst2_f, exp_cst1_f);
    uniform int32 xu1 = tmp & infinity;
    uniform int32 xu2 = (tmp & 0x7fffff) | 0x3f800000;

    uniform float b = floatbits(xu2);

    /* Generated in Sollya with:
       > f=remez(1-x*exp(-(x-1)*log(2)),
                 [|(x-1)*(x-2), (x-1)*(x-2)*x, (x-1)*(x-2)*x*x|],
                 [1.000001,1.999999], exp(-(x-1)*log(2)));
       > plot(exp((x-1)*log(2))/(f+x)-1, [1,2]);
       > f+x;
    */
    return (floatbits(xu1)
            * (0.509871020f
               + b
                     * (0.312146713f
                        + b
                              * (0.166617139f
                                 + b
                                       * (-2.190619930e-3f
                                          + b * 1.3555747234e-2f)))));
}

static inline uniform float log_approx(uniform float val)
{
    uniform float exp = intbits(val) >> 23;
    /* -89.970756366f = -127 * log(2) + constant term of polynomial bellow. */
    uniform float addcst = select(val > 0, -89.970756366f, -(float)infinity);
    uniform float x = floatbits((intbits(val) & 0x7fffff) | 0x3f800000);

    /* Generated in Sollya using:
      > f = remez(log(x)-(x-1)*log(2),
              [|1,(x-1)*(x-2), (x-1)*(x-2)*x, (x-1)*(x-2)*x*x,
                (x-1)*(x-2)*x*x*x|], [1,2], 1, 1e-8);
      > plot(f+(x-1)*log(2)-log(x), [1,2]);
      > f+(x-1)*log(2)
   */
    return (
        x
            * (3.529304993f
               + x
                     * (-2.461222105f
                        + x
                              * (1.130626167f
                                 + x * (-0.288739945f + x * 3.110401639e-2f))))
        + (addcst + 0.6931471805f * exp));
}

#endif /* _REGURGITATE_IMPL_MATH_APPROXIMATIONS_H_ */
