from mamba import description, before, after, it
from expects import *
import os

import client.api
import client.models
from common import Config, Service
from common.matcher import be_valid_module, raise_api_exception


CONFIG = Config(os.path.join(os.path.dirname(__file__), os.environ.get('MAMBA_CONFIG', 'config.yaml')))


with description('Modules, ') as self:
    with before.all:
        service = Service(CONFIG.service())
        self.process = service.start()
        self.api = client.api.ModulesApi(service.client())

    with description('list, '):
        with description('all, '):
            with it('returns list of modules'):
                modules = self.api.list_modules()
                expect(modules).not_to(be_empty)
                for module in modules:
                    expect(module).to(be_valid_module)

            with description('unsupported method, '):
                with it('returns 405'):
                    expect(lambda: self.api.api_client.call_api('/modules', 'PUT')).to(raise_api_exception(
                    405, headers={'Allow': "GET"}))

    with description('get, '):
        with description('known module, '):
            with description('packetio'):
                with it('succeeds'):
                    module = self.api.get_module('packetio')
                    expect(module).to(be_valid_module)
                    expect(module.id).to(equal('packetio'))
                    expect(module.version.version).to(equal(1))

        with description('non-existent module, '):
            with it('returns 404'):
                expect(lambda: self.api.get_module('insertnotfoundmodulenamehere')).to(raise_api_exception(404))

        with description('unsupported method, '):
            with it('returns 405'):
                expect(lambda: self.api.api_client.call_api('/modules/hi', 'PUT')).to(raise_api_exception(
                    405, headers={'Allow': "GET"}))

        with description('invalid module name, '):
            with it('returns 400'):
                expect(lambda: self.api.get_module('and&').to(raise_api_exception(400)))
