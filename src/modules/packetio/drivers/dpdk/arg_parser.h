#ifndef _ICP_PACKETIO_DPDK_ARG_PARSER_H_
#define _ICP_PACKETIO_DPDK_ARG_PARSER_H_

#include <string>
#include <vector>
#include <unordered_map>

namespace icp::packetio::dpdk {

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
    int test_portpairs();                          /**< Number of eth ring devs */
    bool test_mode();                              /**< test mode enable/disable */
    std::vector<std::string> args();               /**< Retrieve a copy of args for use */
    std::unordered_map<int, std::string> id_map(); /**< Retrieve a copy of port idx->id map */

private:
    std::string m_name;
};


} /* namespace icp::packetio::dpdk */


#endif /* _ICP_PACKETIO_DPDK_ARG_PARSER_H_ */
