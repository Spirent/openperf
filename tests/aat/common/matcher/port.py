from expects import *
from expects.matchers import Matcher

import client.models


class _be_valid_port(Matcher):
    def _match(self, p):
        expect(p).to(be_a(client.models.Port))
        expect(p.id).not_to(be_empty)
        expect(['dpdk', 'host', 'bond']).to(contain(p.kind))
        expect(p.config).not_to(be_none)
        if p.kind == 'dpdk':
            expect(p.config.dpdk).not_to(be_none)
            expect(p.config.dpdk.driver).not_to(be_none)
            expect(p.config.dpdk.device).not_to(be_none)
            expect(p.config.dpdk.link).not_to(be_none)
            expect(p.config.dpdk.link.speed).to(be_above_or_equal(0))
            expect(['full', 'half', 'unknown']).to(contain(p.config.dpdk.link.duplex))
        elif p.kind == 'host':
            pass
        elif p.kind == 'bond':
            expect(p.config.bond).not_to(be_none)
            expect(['lag_802_3_ad']).to(contain(p.config.bond.mode))
            expect(p.config.bond.ports).not_to(be_empty)
        else:
            raise AssertionError('unhandled port kind: %s' % p.kind)
        expect(p.status).not_to(be_none)
        expect(['up', 'down', 'unknown']).to(contain(p.status.link))
        expect(p.status.speed).to(be_above_or_equal(0))
        expect(['full', 'half', 'unknown']).to(contain(p.status.duplex))
        expect(p.stats).not_to(be_none)
        return True, ['is valid port']

be_valid_port = _be_valid_port()
