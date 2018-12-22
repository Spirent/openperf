#ifndef _ICP_PACKETIO_DPDK_ARG_PARSER_H_
#define _ICP_PACKETIO_DPDK_ARG_PARSER_H_

#include <string>
#include <vector>

namespace icp {
namespace packetio {
namespace dpdk {

template <typename T>
class singleton {
public:
    static T& instance()
    {
        static T instance;
        return instance;
    }

    singleton(const singleton&) = delete;
    singleton& operator= (const singleton) = delete;

protected:
    singleton() {};
};

class arg_parser : public singleton<arg_parser>
{
public:
    int init(const char *name);               /**< Initialize args vector */
    int parse(int opt, const char *opt_arg);  /**< Parse arguments */
    std::vector<std::string> args();          /**< Retrieve a copy of args for use */

private:
    std::vector<std::string> _args;
};


}
}
}

#endif /* _ICP_PACKETIO_DPDK_ARG_PARSER_H_ */
