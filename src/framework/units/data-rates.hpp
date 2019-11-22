#ifndef _OP_UNITS_DATA_RATES_HPP_
#define _OP_UNITS_DATA_RATES_HPP_

#include "units/rate.hpp"

namespace openperf::units {

/**
 * Convenience typedefs for standard network rates
 **/

typedef std::kilo kilobits;
typedef std::ratio<1024L> kibibits;
typedef std::mega megabits;
typedef std::ratio<1024L * 1024L> mebibits;
typedef std::giga gigabits;
typedef std::ratio<1024L * 1024L * 1024L> gibibits;
typedef std::tera terabits;
typedef std::ratio<1024L * 1024L * 1024L * 1024L> tebibits;

typedef std::ratio<8> bytes;
typedef std::ratio_multiply<kilobits, std::ratio<8>> kilobytes;
typedef std::ratio_multiply<kibibits, std::ratio<8>> kibibytes;
typedef std::ratio_multiply<megabits, std::ratio<8>> megabytes;
typedef std::ratio_multiply<mebibits, std::ratio<8>> mebibytes;
typedef std::ratio_multiply<gigabits, std::ratio<8>> gigabytes;
typedef std::ratio_multiply<gibibits, std::ratio<8>> gibibytes;
typedef std::ratio_multiply<terabits, std::ratio<8>> terabytes;
typedef std::ratio_multiply<tebibits, std::ratio<8>> tebibytes;

} // namespace openperf::units

#endif /* _OP_UNITS_DATA_RATES_HPP_ */
