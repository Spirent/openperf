from expects import *

import client.api
import client.models
import random


def empty_interface(api_client):
    ports = client.api.PortsApi(api_client)
    ps = ports.list_ports(kind='dpdk')
    expect(ps).not_to(be_empty)
    i = client.models.Interface()
    i.port_id = ps[0].id
    i.config = client.models.InterfaceConfig()
    return i


def example_interface(api_client):
    i = empty_interface(api_client)
    i.config.protocols = map(as_interface_protocol, [
        client.models.InterfaceProtocolConfigEth(mac_address='00:00:00:00:00:01'),
        client.models.InterfaceProtocolConfigIpv4(method='static', static=client.models.InterfaceProtocolConfigIpv4Static(address='1.1.1.1', prefix_length=24)),
    ])
    return i


def random_mac(port_id):
    octets = list()
    octets.append(random.randint(0, 255) & 0xfc)
    octets.append((int(port_id) >> 16) & 0xff)
    octets.append(int(port_id) & 0xff)
    for _i in range(3):
        octets.append(random.randint(0, 255))

    return '{0:02x}:{1:02x}:{2:02x}:{3:02x}:{4:02x}:{5:02x}'.format(*octets)


def ipv4_interface(api_client, **kwargs):
    i = empty_interface(api_client)
    if 'ipv4_address' in kwargs:
        i.config.protocols = map(as_interface_protocol, [
            client.models.InterfaceProtocolConfigEth(mac_address=random_mac(i.port_id)),
            client.models.InterfaceProtocolConfigIpv4(
                method='static',
                static=client.models.InterfaceProtocolConfigIpv4Static(
                    address=kwargs['ipv4_address'],
                    prefix_length=kwargs['prefix_length'] if 'prefix_length' in kwargs else 24,
                    gateway=kwargs['gateway'] if 'gateway' in kwargs else None))
        ])
    else:
        i.config.protocols = map(as_interface_protocol, [
            client.models.InterfaceProtocolConfigEth(mac_address=random_mac(i.port_id)),
            client.models.InterfaceProtocolConfigIpv4(
                method='dhcp',
                dhcp=client.models.InterfaceProtocolConfigIpv4Dhcp())
        ])
    return i


def as_interface_protocol(p):
    expect(p).not_to(be_none)
    name = p.__class__.__name__
    expect(name).to(start_with('InterfaceProtocolConfig'))
    proto = name[23:].lower()
    expect(proto).not_to(be_empty)
    pc = client.models.InterfaceProtocolConfig()
    expect(pc).to(have_property(proto))
    setattr(pc, proto, p)
    return pc
