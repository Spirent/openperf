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
        #not required version properties. make sure they are sent though.
        expect(m.version).to(have_property('build_number'))
        expect(m.version).to(have_property('build_date'))
        expect(m.version).to(have_property('source_commit'))

        #linkage
        expect(['static', 'dynamic']).to(contain(m.linkage))
        if m.linkage == 'dynamic':
            expect(m.path).not_to(be_empty)
        else:
            expect(m.path).to(equal(''))

        return True, ['is valid module']

be_valid_module = _be_valid_module()
