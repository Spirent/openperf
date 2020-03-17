from expects import *
from expects.matchers import Matcher

import client.models


class _be_valid_interface(Matcher):
    def _match(self, i):
        expect(i).to(be_a(client.models.Interface))
        expect(i.id).not_to(be_empty)
        expect(i.config).not_to(be_none)
        expect(i.config.protocols).not_to(be_empty)
        for p in i.config.protocols:
            expect(p).to(be_valid_protocol)
        expect(i.stats).not_to(be_none)
        return True, ['is valid interface']

be_valid_interface = _be_valid_interface()


class _be_valid_eth_protocol_config(Matcher):
    def _match(self, p):
        expect(p).to(be_a(client.models.InterfaceProtocolConfigEth))
        expect(p.mac_address).not_to(be_empty)
        return True, ['is valid eth protocol']


class _be_valid_ipv4_protocol_config(Matcher):
    def _match(self, p):
        expect(p).to(be_a(client.models.InterfaceProtocolConfigIpv4))
        return True, ['is valid ipv4 protocol']


class _be_valid_ipv6_protocol_config(Matcher):
    def _match(self, p):
        expect(p).to(be_a(client.models.InterfaceProtocolConfigIpv6))
        return True, ['is valid ipv6 protocol']


class _be_valid_protocol(Matcher):
    _matchers = {
        'eth': _be_valid_eth_protocol_config(),
        'ipv4': _be_valid_ipv4_protocol_config(),
        'ipv6': _be_valid_ipv6_protocol_config(),
    }

    def _match(self, pc):
        expect(pc).to(be_a(client.models.InterfaceProtocolConfig))
        proto = None
        for a in client.models.InterfaceProtocolConfig.attribute_map:
            p = getattr(pc, a)
            if p is not None:
                expect(proto).to(be_none)
                proto = p
                expect(_be_valid_protocol._matchers).to(have_key(a))
                be_valid = _be_valid_protocol._matchers[a]
        expect(proto).not_to(be_none)
        expect(proto).to(be_valid)
        return True, ['is valid protocol']

be_valid_protocol = _be_valid_protocol()
