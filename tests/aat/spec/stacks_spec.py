from mamba import description, before, after, it
from expects import expect, be_empty
import os

import client.api
import client.models
from common import Config, Service
from common.matcher import be_valid_stack, raise_api_exception


CONFIG = Config(os.path.join(os.path.dirname(__file__), os.environ.get('MAMBA_CONFIG', 'config.yaml')))


with description('Stacks,') as self:
    with before.all:
        service = Service(CONFIG.service())
        self.process = service.start()
        self.api = client.api.StacksApi(service.client())

    with description('list,'):
        with it('returns valid stacks'):
            stacks = self.api.list_stacks()
            expect(stacks).not_to(be_empty)
            for stack in stacks:
                expect(stack).to(be_valid_stack)

    with description('get,'):
        with description('known existing stack,'):
            with it('succeeds'):
                expect(self.api.get_stack('0')).to(be_valid_stack)

        with description('non-existent stack,'):
            with it('returns 404'):
                expect(lambda: self.api.get_stack('foo')).to(raise_api_exception(404))

    with after.all:
        try:
            self.process.kill()
            self.process.wait()
        except AttributeError:
            pass
