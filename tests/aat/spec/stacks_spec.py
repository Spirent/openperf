from mamba import description, before, after, it
from common import Config, Service
from common.matcher import be_valid_stack, raise_api_exception
from expects import expect, be_empty

import client.api
import client.models
import os


CONFIG = Config(os.path.join(os.path.dirname(__file__), 'config.yaml'))


with description('Stacks,') as self:
    with before.all:
        service = Service(CONFIG.service())
        self.process = service.start()
        self.stacks = client.api.StacksApi(service.client())

    with description('list,'):
        with it('returns valid stacks'):
            stacks = self.stacks.list_stacks()
            expect(stacks).not_to(be_empty)
            for stack in stacks:
                expect(stack).to(be_valid_stack)

    with description('get,'):
        with description('known existing stack,'):
            with it('succeeds'):
                expect(self.stacks.get_stack('0')).to(be_valid_stack)

        with description('non-existent stack,'):
            with it('returns 404'):
                expect(lambda: self.stacks.get_stack('foo')).to(raise_api_exception(404))
