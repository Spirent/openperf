#ifndef _ICP_PACKETIO_VIF_MAP_H_
#define _ICP_PACKETIO_VIF_MAP_H_

#include <memory>

#include "core/icp_core.h"
#include "net/net_types.h"

namespace icp {
namespace packetio {

template <typename Interface>
class vif_map {
public:
    vif_map();

    bool insert(uint16_t id, const net::mac_address& mac, Interface* ifp);
    bool remove(uint16_t id, const net::mac_address& mac, Interface* ifp);

    Interface*                  find(uint16_t id, const net::mac_address& mac) const;
    Interface*                  find(uint16_t id, const uint8_t octets[6]) const;
    const icp::list<Interface>& find(uint16_t id) const;

    void shrink_to_fit();

private:
    struct icp_hashtab_deleter {
        void operator()(icp_hashtab* tab) const {
            icp_hashtab_free(&tab);
        }
    };

    std::unique_ptr<icp_hashtab, icp_hashtab_deleter> m_interfaces;
    std::unique_ptr<icp_hashtab, icp_hashtab_deleter> m_ports;
    icp::list<Interface> m_empty;
};

}
}
#endif /* _ICP_PACKETIO_VIF_MAP_H_ */
