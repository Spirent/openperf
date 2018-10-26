from expects import expect, be_a, be_empty, be_none
from expects.matchers import Matcher

import client.models


class _be_valid_stack(Matcher):
    def _match(self, stack):
        expect(stack).to(be_a(client.models.Stack))
        expect(stack.id).not_to(be_empty)
        expect(stack.stats).not_to(be_none)
        return True, ['is valid stack']


be_valid_stack = _be_valid_stack()
