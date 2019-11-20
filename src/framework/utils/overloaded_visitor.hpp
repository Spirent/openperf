#ifndef _OP_UTILS_OVERLOADED_VISITOR_H_
#define _OP_UTILS_OVERLOADED_VISITOR_H_

namespace openperf::utils {

/**
 * This struct is magic.  Use templates and parameter packing to provide
 * some syntactic sugar for creating visitor objects for std::visit.
 */
template<typename ...Ts>
struct overloaded_visitor : Ts...
{
    overloaded_visitor(const Ts&... args)
        : Ts(args)...
    {}

    using Ts::operator()...;
};

}

#endif /* _OP_UTILS_OVERLOADED_VISITOR_H_ */
