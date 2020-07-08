from expects import *
from expects.matchers import Matcher
import os
import sys

import client.api
import client.models
from common import Config, Service
from common.matcher import (be_valid_counter,
                            be_valid_module,
                            be_valid_block_device,
                            be_valid_cpu_info,
                            be_valid_memory_info,
                            raise_api_exception)


CONFIG = Config(os.path.join(os.path.dirname(__file__), os.environ.get('MAMBA_CONFIG', 'config.yaml')))

class _has_json_content_type(Matcher):
    def _match(self, request):
        expect(request).to(have_key('Content-Type'))
        expect(request['Content-Type']).to(equal('application/json'))
        return True, ['is JSON content type']

has_json_content_type = _has_json_content_type()

with description('Docker,', 'docker') as self:
    with before.all:
        self.service = Service(CONFIG.service())
        self.process = self.service.start()
        self.time_api = client.api.TimeSyncApi(self.service.client())
        self.modules_api = client.api.ModulesApi(self.service.client())
        self.block_api = client.api.BlockGeneratorApi(self.service.client())
        self.cpu_api = client.api.CpuGeneratorApi(self.service.client())
        self.memory_api = client.api.MemoryGeneratorApi(self.service.client())

    with description('Timesync,', 'timesync'):
        with description('counters,'):
            with description('list,'):
                with it('succeeds'):
                    counters = self.time_api.list_time_counters()
                    expect(counters).not_to(be_empty)
                    for counter in counters:
                        expect(counter).to(be_valid_counter)

            with description('valid counter,'):
                with it('succeeds'):
                    for counter in self.time_api.list_time_counters():
                        expect(self.time_api.get_time_counter(counter.id)).to(be_valid_counter)

            with description('non-existent counter,'):
                with it('returns 404'):
                    expect(lambda: self.time_api.get_time_counter('foo')).to(raise_api_exception(404))

    with description('Modules,', 'modules'):
        with description('list,'):
            with description('all,'):
                with it('returns list of modules'):
                    modules = self.modules_api.list_modules()
                    expect(modules).not_to(be_empty)
                    for module in modules:
                        expect(module).to(be_valid_module)

    with description('CPU Generator Module,', 'cpu'):
        with description('cpu info,'):
            with before.all:
                self._result = self.cpu_api.cpu_info_with_http_info(
                    _return_http_data_only=False)

            with it('succeeded'):
                expect(self._result[1]).to(equal(200))

            with it('has application/json header'):
                expect(self._result[2]).to(has_json_content_type)

            with it('valid cpu info'):
                expect(self._result[0]).to(be_valid_cpu_info)

    with description('Memory Generator Module,', 'memory'):
        with description('memory info,'):
            with before.all:
                self._result = self.memory_api.memory_info_with_http_info(
                    _return_http_data_only=False)

            with it('succeeded'):
                expect(self._result[1]).to(equal(200))

            with it('has application/json header'):
                expect(self._result[2]).to(has_json_content_type)

            with it('valid memory info'):
                expect(self._result[0]).to(be_valid_memory_info)

    with description('Block,', 'block'):
        with description('list devices,'):
            with it('succeeds'):
                devices = self.block_api.list_block_devices()
                expect(devices).not_to(be_empty)
                for dev in devices:
                    expect(dev).to(be_valid_block_device)

    with after.all:
        try:
            self.service.stop()
            self.process.terminate()
            self.process.join()
        except AttributeError:
            pass
