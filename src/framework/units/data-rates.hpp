#ifndef _OP_UNITS_DATA_RATES_HPP_
#define _OP_UNITS_DATA_RATES_HPP_

#include "units/rate.hpp"

namespace openperf::units {

/**
 * Convenience typedefs for standard network rates
 **/

using kilobits = std::kilo;
using kibibits = std::ratio<1024L>;
using megabits = std::mega;
using mebibits = std::ratio<1024L * 1024L>;
using gigabits = std::giga;
using gibibits = std::ratio<1024L * 1024L * 1024L>;
using terabits = std::tera;
using tebibits = std::ratio<1024L * 1024L * 1024L * 1024L>;

using bytes = std::ratio<8>;
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
