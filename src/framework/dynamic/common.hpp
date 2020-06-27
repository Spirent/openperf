#ifndef _OP_DYNAMIC_COMMON_HPP_
#define _OP_DYNAMIC_COMMON_HPP_

#include <string>

namespace openperf::dynamic {

struct dynamic_argument
{
    enum function_t { DX = 1, DXDY, DXDT };
    std::string x;
    std::string y;
    function_t function;

    // dynamic_argument(std::string_view x_name, function_t f = DX)
    //    : x(x_name)
    //    , function(f)
    //{}

    // dynamic_argument(std::string_view x_name, std::string_view y_name)
    //    : x(x_name)
    //    , y(y_name)
    //    , function(DXDY)
    //{}
};

} // namespace openperf::dynamic

#endif // _OP_DYNAMIC_COMMON_HPP_
