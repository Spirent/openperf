#ifndef _OP_CPU_ISPC_HPP_
#define _OP_CPU_ISPC_HPP_

namespace ispc {

/* Turn our nasty ifdef's into nice boolean constants */
#ifdef ISPC_TARGET_AUTOMATIC
constexpr bool automatic_enabled = true;
#else
constexpr bool automatic_enabled = false;
#endif

#ifdef ISPC_TARGET_SSE2
constexpr bool sse2_enabled = true;
#else
constexpr bool sse2_enabled = false;
#endif

#ifdef ISPC_TARGET_SSE4
constexpr bool sse4_enabled = true;
#else
constexpr bool sse4_enabled = false;
#endif

#ifdef ISPC_TARGET_AVX
constexpr bool avx_enabled = true;
#else
constexpr bool avx_enabled = false;
#endif

#ifdef ISPC_TARGET_AVX2
constexpr bool avx2_enabled = true;
#else
constexpr bool avx2_enabled = false;
#endif

#ifdef ISPC_TARGET_AVX512SKX
constexpr bool avx512skx_enabled = true;
#else
constexpr bool avx512skx_enabled = false;
#endif

} // namespace ispc

#endif // _OP_CPU_ISPC_HPP_
