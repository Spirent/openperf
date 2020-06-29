#ifndef _OP_DYNAMIC_COMMON_HPP_
#define _OP_DYNAMIC_COMMON_HPP_

#include <string>

namespace openperf::dynamic {

struct argument
{
    enum function_t { DX = 1, DXDY, DXDT };
    std::string x;
    std::string y;
    function_t function;
};

} // namespace openperf::dynamic

#endif // _OP_DYNAMIC_COMMON_HPP_
