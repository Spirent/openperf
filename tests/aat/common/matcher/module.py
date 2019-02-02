from expects import *
from expects.matchers import Matcher

import client.models


class _be_valid_module(Matcher):
    def _match(self, m):
        expect(m).to(be_a(client.models.Module))
        expect(m.id).not_to(be_empty)
        expect(m.description).not_to(be_empty)

        #version
        expect(m.version).to(be_a(client.models.ModuleVersion))
        expect(m.version.version).to(be_above_or_equal(0))

        if hasattr(m, 'build_number'):
            expect(m.version.build_number).to_not(be_empty())
        if hasattr(m, 'build_date'):
            expect(m.version.build_date).to_not(be_empty())
        if hasattr(m, 'source_commit'):
            expect(m.version.source_commit).to_not(be_empty())


        #linkage
        expect(['static', 'dynamic']).to(contain(m.linkage))
        if m.linkage == 'dynamic':
            expect(m.path).not_to(be_empty)
        else:
            expect(m.path).to(equal(''))

        return True, ['is valid module']

be_valid_module = _be_valid_module()
