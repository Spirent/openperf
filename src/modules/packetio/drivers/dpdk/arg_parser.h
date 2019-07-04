#ifndef _ICP_PACKETIO_DPDK_ARG_PARSER_H_
#define _ICP_PACKETIO_DPDK_ARG_PARSER_H_

#include <string>
#include <vector>
#include <unordered_map>

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
    int init(const char *name);                    /**< Initialize args vector */
    //int parse(int opt, const char *opt_arg);       /**< Parse arguments */
    int test_portpairs();                          /**< Number of eth ring devs */
    bool test_mode();                              /**< test mode enable/disable */
    std::vector<std::string> args();               /**< Retrieve a copy of args for use */
    std::unordered_map<int, std::string> id_map(); /**< Retrieve a copy of port idx->id map */

private:
    std::vector<std::string> _args;
    std::string m_name;
    //int m_test_portpairs;
    //bool m_test_mode;
    //std::unordered_map<int, std::string> m_port_index_id;
};


}
}
}

#endif /* _ICP_PACKETIO_DPDK_ARG_PARSER_H_ */
