#ifndef __BYTE_SWAP_HPP__
#define __BYTE_SWAP_HPP__

namespace openperf::net {

#if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__

inline uint64_t htonll(uint64_t v) { return v; }
inline uint64_t ntohll(uint64_t v) { return v; }

#elif __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__

inline uint64_t htonll(uint64_t v) { return __builtin_bswap64(v); }
inline uint64_t ntohll(uint64_t v) { return __builtin_bswap64(v); }

#else

#error Unable to determine byte order

#endif

} // namespace openperf::net

#endif // __BYTE_SWAP_HPP__