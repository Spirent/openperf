#include <functional>
#include <limits>

#include "packetio/vif_map.h"

namespace icp {
namespace packetio {

static uintptr_t interface_hasher(const void *port_mac)
{
    return (static_cast<uintptr_t>(
                std::hash<uint64_t>()(
                    reinterpret_cast<const uint64_t>(port_mac))));
}

static uintptr_t port_hasher(const void *port)
{
    return (static_cast<uintptr_t>(
                std::hash<uint16_t>()(
                    reinterpret_cast<const uintptr_t>(port)
                    & std::numeric_limits<uint16_t>::max())));
}

template <typename Interface>
static void port_list_destructor(void *port_list)
{
    delete reinterpret_cast<icp::list<Interface>*>(port_list);
}

static void* __attribute__((const)) to_key(uint16_t id, const net::mac_address& mac)
{
    return (reinterpret_cast<void*>(
                (static_cast<uintptr_t>(id) << 48
                 | static_cast<uintptr_t>(mac[0]) << 40
                 | static_cast<uintptr_t>(mac[1]) << 32
                 | mac[2] << 24
                 | mac[3] << 16
                 | mac[4] << 8
                 | mac[5])));
}

static void* __attribute__((const)) to_key(uint16_t id, const uint8_t octets[6])
{
    return (reinterpret_cast<void*>(
                (static_cast<uintptr_t>(id) << 48
                 | static_cast<uintptr_t>(octets[0]) << 40
                 | static_cast<uintptr_t>(octets[1]) << 32
                 | octets[2] << 24
                 | octets[3] << 16
                 | octets[4] << 8
                 | octets[5])));
}

static void* __attribute__((const)) to_key(uint16_t id)
{
    return (reinterpret_cast<void*>(id));
}

template <typename Interface>
vif_map<Interface>::vif_map()
    : m_interfaces(icp_hashtab_allocate())
    , m_ports(icp_hashtab_allocate())
{
    icp_hashtab_set_hasher(m_interfaces.get(), interface_hasher);

    icp_hashtab_set_hasher(m_ports.get(), port_hasher);
    icp_hashtab_set_value_destructor(m_ports.get(), port_list_destructor<Interface>);
}

template <typename Interface>
bool vif_map<Interface>::insert(uint16_t id, const net::mac_address& mac, Interface* ifp)
{
    if (icp_hashtab_find(m_ports.get(), to_key(id)) == nullptr) {
        icp_hashtab_insert(m_ports.get(), to_key(id), new icp::list<Interface>());
    }

    auto port_list = reinterpret_cast<icp::list<Interface>*>(icp_hashtab_find(m_ports.get(), to_key(id)));

    bool success = (port_list->insert(ifp)
                    && icp_hashtab_insert(m_interfaces.get(), to_key(id, mac), ifp));
    if (!success) {
        remove(id, mac, ifp);
    }

    return (success);
}

template <typename Interface>
bool vif_map<Interface>::remove(uint16_t id, const net::mac_address& mac, Interface* ifp)
{
    bool success = false;
    if (auto tmp = icp_hashtab_find(m_ports.get(), to_key(id))) {
        auto port_list = reinterpret_cast<icp::list<Interface>*>(tmp);
        success = port_list->remove(ifp);
    }

    success &= icp_hashtab_delete(m_interfaces.get(), to_key(id, mac));

    return (success);
}

template <typename Interface>
Interface* vif_map<Interface>::find(uint16_t id, const net::mac_address& mac) const
{
    return (reinterpret_cast<Interface*>(icp_hashtab_find(m_interfaces.get(),
                                                          to_key(id, mac))));
}

template <typename Interface>
Interface* vif_map<Interface>::find(uint16_t id, const uint8_t octets[6]) const
{
    return (reinterpret_cast<Interface*>(icp_hashtab_find(m_interfaces.get(),
                                                          to_key(id, octets))));
}

template <typename Interface>
const icp::list<Interface>& vif_map<Interface>::find(uint16_t id) const
{
    auto port_list = icp_hashtab_find(m_ports.get(), to_key(id));
    return (port_list == nullptr
            ? m_empty
            : *(reinterpret_cast<icp::list<Interface>*>(port_list)));
}

template <typename Interface>
void vif_map<Interface>::shrink_to_fit()
{
    icp_hashtab_garbage_collect(m_interfaces.get());

    void* cursor = nullptr;
    void *current = icp_hashtab_iterate(m_ports.get(), &cursor);
    while (current != nullptr) {
        auto port_list = reinterpret_cast<icp::list<Interface>*>(current);
        port_list->shrink_to_fit();
        current = icp_hashtab_iterate(m_ports.get(), &cursor);
    }
}

}
}
