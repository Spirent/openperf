from expects import *

import client.api
import client.models


def example_interface(api_client):
    ports = client.api.PortsApi(api_client)
    ps = ports.list_ports(kind='dpdk')
    expect(ps).not_to(be_empty)
    i = client.models.Interface()
    i.port_id = ps[0].id
    i.config = client.models.InterfaceConfig()
    i.config.protocols = map(as_interface_protocol, [
        client.models.InterfaceProtocolConfigEth(mac_address='00:00:00:00:00:01'),
        client.models.InterfaceProtocolConfigIpv4(method='static', static=client.models.InterfaceProtocolConfigIpv4Static(address='1.1.1.1', prefix_length=24)),
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
