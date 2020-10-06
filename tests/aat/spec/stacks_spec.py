from mamba import description, before, after, it
from expects import expect, be_empty
import os

import client.api
import client.models
from common import Config, Service
from common.matcher import be_valid_stack, raise_api_exception
from common.helper import check_modules_exists


CONFIG = Config(os.path.join(os.path.dirname(__file__), os.environ.get('MAMBA_CONFIG', 'config.yaml')))


with description('Stacks,', 'stacks') as self:
    with before.all:
        service = Service(CONFIG.service())
        self.process = service.start()
        self.api = client.api.StacksApi(service.client())
        if not check_modules_exists(service.client(), 'packet-stack'):
            self.skip()

    with description('list,'):
        with it('returns valid stacks'):
            stacks = self.api.list_stacks()
            expect(stacks).not_to(be_empty)
            for stack in stacks:
                expect(stack).to(be_valid_stack)

        with description('unsupported method'):
            with it('returns 405'):
                expect(lambda: self.api.api_client.call_api('/stacks', 'PUT')).to(raise_api_exception(
                    405, headers={'Allow': "GET"}))

    with description('get,'):
        with description('known existing stack,'):
            with it('succeeds'):
                expect(self.api.get_stack('stack0')).to(be_valid_stack)

        with description('non-existent stack,'):
            with it('returns 404'):
                expect(lambda: self.api.get_stack('foo')).to(raise_api_exception(404))

        with description('unsupported method'):
            with it('returns 405'):
                expect(lambda: self.api.api_client.call_api('/stacks/0', 'PUT')).to(raise_api_exception(
                    405, headers={'Allow': "GET"}))

    with after.all:
        try:
            self.process.terminate()
            self.process.wait()
        except AttributeError:
            pass
