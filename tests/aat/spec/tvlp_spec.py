import os
import client.api
import client.models
import datetime

from mamba import description, before, after
from expects import *
from expects.matchers import Matcher
from common import Config, Service
from common.helper import check_modules_exists

# Block generator
from common.helper import (check_modules_exists,
                           file_model,
                           bulk_start_model,
                           bulk_stop_model,
                           wait_for_file_initialization_done)


from common.matcher import (be_valid_block_file,
                            be_valid_block_generator,
                            be_valid_block_generator_result,
                            be_valid_memory_generator,
                            be_valid_memory_generator_result,
                            be_valid_cpu_generator,
                            be_valid_cpu_generator_result,
                            has_location,
                            raise_api_exception)

# TVLP
from common.helper import (tvlp_model,
                           tvlp_block_profile_model,
                           tvlp_memory_profile_model,
                           tvlp_cpu_profile_model)

from common.matcher import (be_valid_tvlp_configuration,
                            be_valid_block_tvlp_profile,
                            be_valid_memory_tvlp_profile,
                            be_valid_cpu_tvlp_profile,
                            be_valid_tvlp_result)


CONFIG = Config(os.path.join(os.path.dirname(__file__),
                             os.environ.get('MAMBA_CONFIG', 'config.yaml')))

def wait_for_tvlp_state(api_client, id, state, timeout):
    for i in range(timeout * 10):
        f = api_client.get_tvlp_configuration(id)
        expect(f.state).not_to(equal("error"))
        if f.state == state:
            return
        time.sleep(.1)
    expect(False).to(be_true)

with description('TVLP,', 'tvlp') as self:
    with description('REST API,'):
        with before.all:
            self.service = Service(CONFIG.service())
            self.process = self.service.start()
            self.block_api = client.api.BlockGeneratorApi(self.service.client())
            self.memory_api = client.api.MemoryGeneratorApi(self.service.client())
            self.cpu_api = client.api.CpuGeneratorApi(self.service.client())
            self.tvlp_api = client.api.TVLPApi(self.service.client())
            if not check_modules_exists(self.service.client(), 'tvlp'):
                self.skip()
            self._block_linked = check_modules_exists(self.service.client(), 'block')
            self._memory_linked = check_modules_exists(self.service.client(), 'memory')
            self._cpu_linked = check_modules_exists(self.service.client(), 'cpu')

        with description('create,'):
            with description("succeeds,"):
                with before.all:
                    t = tvlp_model()
                    if self._block_linked:
                        t.profile.block = tvlp_block_profile_model(1, 100000000, "tmp")
                    if self._memory_linked:
                        t.profile.memory = tvlp_memory_profile_model(1, 100000000)
                    if self._cpu_linked:
                        t.profile.cpu = tvlp_cpu_profile_model(1, 100000000)
                    self._config = self.tvlp_api.create_tvlp_configuration_with_http_info(t)

                with it('code Created'):
                    expect(self._config[1]).to(equal(201))

                with it('has valid Location header,'):
                    expect(self._config[2]).to(has_location('/tvlp/' + self._config[0].id))

                with it('has a valid tvlp configuration'):
                    expect(self._config[0]).to(be_valid_tvlp_configuration)

                with it('has a valid block profile'):
                    if not self._block_linked:
                        self.skip()
                    expect(self._config[0].profile.block).to(be_valid_block_tvlp_profile)

                with it('has a valid memory profile'):
                    if not self._memory_linked:
                        self.skip()
                    expect(self._config[0].profile.memory).to(be_valid_memory_tvlp_profile)

                with it('has a valid cpu profile'):
                    if not self._cpu_linked:
                        self.skip()
                    expect(self._config[0].profile.cpu).to(be_valid_cpu_tvlp_profile)

            with description("no profile entries,"):
                with it('returns 400'):
                    expect(lambda: self.tvlp_api.create_tvlp_configuration(tvlp_model())).to(raise_api_exception(400))

            with description('empty resource id,'):
                with description('block,'):
                    with it('returns 400'):
                        if not self._block_linked:
                            self.skip()
                        t = tvlp_model()
                        t.profile.block = tvlp_block_profile_model(1, 100000000, "tmp")
                        t.profile.block.series[0].resource_id = None
                        expect(lambda: self.tvlp_api.create_tvlp_configuration(t)).to(raise_api_exception(400))

        with description('list,'):
            with before.each:
                t = tvlp_model()
                if self._block_linked:
                    t.profile.block = tvlp_block_profile_model(1, 100000000, "tmp")
                if self._memory_linked:
                    t.profile.memory = tvlp_memory_profile_model(1, 100000000)
                if self._cpu_linked:
                    t.profile.cpu = tvlp_cpu_profile_model(1, 100000000)
                self._config = self.tvlp_api.create_tvlp_configuration_with_http_info(t)

            with it('succeeds'):
                configurations = self.tvlp_api.list_tvlp_configurations()
                expect(configurations).not_to(be_empty)
                expect(len(configurations)).to(equal(1))
                for gen in configurations:
                    expect(gen).to(be_valid_tvlp_configuration)

        with description('get,'):
            with description('by existing id,'):
                with before.each:
                    t = tvlp_model()
                    if self._block_linked:
                        t.profile.block = tvlp_block_profile_model(1, 100000000, "tmp")
                    if self._memory_linked:
                        t.profile.memory = tvlp_memory_profile_model(1, 100000000)
                    if self._cpu_linked:
                        t.profile.cpu = tvlp_cpu_profile_model(1, 100000000)
                    self._config = self.tvlp_api.create_tvlp_configuration(t)

                with it('succeeds'):
                    expect(self.tvlp_api.get_tvlp_configuration(self._config.id)).to(be_valid_tvlp_configuration)

            with description('non-existent configuration,'):
                with it('returns 404'):
                    expect(lambda: self.tvlp_api.get_tvlp_configuration('foo')).to(raise_api_exception(404))

            with description('invalid id,'):
                with it('returns 400'):
                    expect(lambda: self.tvlp_api.get_tvlp_configuration('f_oo')).to(raise_api_exception(400))

        with description('delete,'):
            with description('by existing id,'):
                with before.each:
                    t = tvlp_model()
                    if self._block_linked:
                        t.profile.block = tvlp_block_profile_model(1, 100000000, "tmp")
                    if self._memory_linked:
                        t.profile.memory = tvlp_memory_profile_model(1, 100000000)
                    if self._cpu_linked:
                        t.profile.cpu = tvlp_cpu_profile_model(1, 100000000)
                    self._config = self.tvlp_api.create_tvlp_configuration(t)

                with it('succeeds'):
                    self.tvlp_api.delete_tvlp_configuration(self._config.id)
                    confs = self.tvlp_api.list_tvlp_configurations()
                    expect(confs).to(be_empty)

                with description('non-existent id,'):
                    with it('returns 404'):
                        expect(lambda: self.tvlp_api.delete_tvlp_configuration("foo")).to(raise_api_exception(404))

                with description('invalid id,'):
                    with it('returns 400'):
                        expect(lambda: self.tvlp_api.delete_tvlp_configuration("invalid_id")).to(raise_api_exception(400))

        with description('start,'):
            with before.each:
                file = self.block_api.create_block_file(file_model(1024, '/tmp/foo'))
                expect(file).to(be_valid_block_file)
                self.file = file
                wait_for_file_initialization_done(self.block_api, file.id, 1)

                t = tvlp_model()
                if self._block_linked:
                    t.profile.block = tvlp_block_profile_model(1, 100000000, file.id)
                if self._memory_linked:
                    t.profile.memory = tvlp_memory_profile_model(1, 100000000)
                if self._cpu_linked:
                    t.profile.cpu = tvlp_cpu_profile_model(1, 100000000)
                self._config = self.tvlp_api.create_tvlp_configuration(t)

            with description('by existing id,'):
                with it('succeeded'):
                    result = self.tvlp_api.start_tvlp_configuration_with_http_info(self._config.id)
                    expect(result[1]).to(equal(201))
                    expect(result[2]).to(has_location('/tvlp-results/' + result[0].id))
                    expect(result[0]).to(be_valid_tvlp_result)

            with description('delayed,'):
                with it('succeeded'):
                    next_day = datetime.datetime.today() + datetime.timedelta(days=2)
                    fmt_day = next_day.strftime("%Y-%m-%dT%H:%M:%S")
                    result = self.tvlp_api.start_tvlp_configuration_with_http_info(self._config.id, time=str(fmt_day))
                    expect(result[1]).to(equal(201))
                    expect(result[2]).to(has_location('/tvlp-results/' + result[0].id))
                    expect(result[0]).to(be_valid_tvlp_result)
                    config = self.tvlp_api.get_tvlp_configuration(self._config.id)
                    expect(config).to(be_valid_tvlp_configuration)
                    expect(config.state).to(equal("countdown"))

            with description('running,'):
                with it('returns 400'):
                    self.tvlp_api.start_tvlp_configuration(self._config.id)
                    expect(lambda: self.tvlp_api.start_tvlp_configuration(self._config.id)).to(raise_api_exception(400))

            with description('non-existent id,'):
                with it('returns 404'):
                    expect(lambda: self.tvlp_api.start_tvlp_configuration('foo')).to(raise_api_exception(404))

            with description('invalid id,'):
                with it('returns 400'):
                    expect(lambda: self.tvlp_api.start_tvlp_configuration('f_oo')).to(raise_api_exception(400))

        with description('stop running,'):
            with before.each:
                file = self.block_api.create_block_file(file_model(1024, '/tmp/foo'))
                expect(file).to(be_valid_block_file)
                self.file = file
                wait_for_file_initialization_done(self.block_api, file.id, 1)

                t = tvlp_model()
                if self._block_linked:
                    t.profile.block = tvlp_block_profile_model(1, 100000000, file.id)
                if self._memory_linked:
                    t.profile.memory = tvlp_memory_profile_model(1, 100000000)
                if self._cpu_linked:
                    t.profile.cpu = tvlp_cpu_profile_model(1, 100000000)
                self._config = self.tvlp_api.create_tvlp_configuration(t)

                result = self.tvlp_api.start_tvlp_configuration_with_http_info(self._config.id)
                expect(result[1]).to(equal(201))

            with description('by existing id,'):
                with it('succeeds'):
                    self.tvlp_api.stop_tvlp_configuration(self._config.id)
                    expect(self.tvlp_api.get_tvlp_configuration(self._config.id).state).to(equal("ready"))

            with description('non-existent id,'):
                with it('returns 404'):
                    expect(lambda: self.tvlp_api.stop_tvlp_configuration('foo')).to(raise_api_exception(404))

            with description('invalid id,'):
                with it('returns 400'):
                    expect(lambda: self.tvlp_api.stop_tvlp_configuration('f_oo')).to(raise_api_exception(400))

        with description('list results,'):
            with before.each:
                file = self.block_api.create_block_file(file_model(1024, '/tmp/foo'))
                expect(file).to(be_valid_block_file)
                self.file = file
                wait_for_file_initialization_done(self.block_api, file.id, 1)

                t = tvlp_model()
                if self._block_linked:
                    t.profile.block = tvlp_block_profile_model(1, 100000000, file.id)
                if self._memory_linked:
                    t.profile.memory = tvlp_memory_profile_model(1, 100000000)
                if self._cpu_linked:
                    t.profile.cpu = tvlp_cpu_profile_model(1, 100000000)
                self._config = self.tvlp_api.create_tvlp_configuration(t)

                self._result = self.tvlp_api.start_tvlp_configuration_with_http_info(self._config.id)
                expect(self._result[1]).to(equal(201))

            with description('unfiltered,'):
                with it('succeeds'):
                    results = self.tvlp_api.list_tvlp_results()
                    expect(results).not_to(be_empty)
                    for result in results:
                        expect(result).to(be_valid_tvlp_result)

            with description('by id,'):
                with it('succeeds'):
                    res = self.tvlp_api.get_tvlp_result(self._result[0].id)
                    expect(res).to(be_valid_tvlp_result)

            with description('non-existent id,'):
                with it('returns 404'):
                    expect(lambda: self.tvlp_api.get_tvlp_result('foo')).to(raise_api_exception(404))

            with description('invalid id,'):
                with it('returns 400'):
                    expect(lambda: self.tvlp_api.get_tvlp_result('f_oo')).to(raise_api_exception(400))

        with description('results valid,'):
            with before.all:
                file = self.block_api.create_block_file(file_model(1024, '/tmp/foo'))
                expect(file).to(be_valid_block_file)
                self.file = file
                wait_for_file_initialization_done(self.block_api, file.id, 1)

                t = tvlp_model()
                if self._block_linked:
                    t.profile.block = tvlp_block_profile_model(1, 100000000, file.id)
                if self._memory_linked:
                    t.profile.memory = tvlp_memory_profile_model(1, 100000000)
                if self._cpu_linked:
                    t.profile.cpu = tvlp_cpu_profile_model(1, 100000000)
                self._config = self.tvlp_api.create_tvlp_configuration(t)

                result = self.tvlp_api.start_tvlp_configuration_with_http_info(self._config.id)
                expect(result[1]).to(equal(201))

                wait_for_tvlp_state(self.tvlp_api, self._config.id, "ready", 5)
                self._result = self.tvlp_api.get_tvlp_result(result[0].id)
                expect(self._result).to(be_valid_tvlp_result)

            with it('has a valid block results'):
                if not self._block_linked:
                    self.skip()
                expect(self._result.block).not_to(be_empty)
                for result in self._result.block:
                    expect(result).to(be_valid_block_generator_result)

            with it('has a valid memory results'):
                if not self._memory_linked:
                    self.skip()
                expect(self._result.memory).not_to(be_empty)
                for result in self._result.memory:
                    expect(result).to(be_valid_memory_generator_result)

            with it('has a valid cpu results'):
                if not self._cpu_linked:
                    self.skip()
                expect(self._result.cpu).not_to(be_empty)
                for result in self._result.cpu:
                    expect(result).to(be_valid_cpu_generator_result)

        with description('delete results,'):
            with before.each:
                file = self.block_api.create_block_file(file_model(1024, '/tmp/foo'))
                expect(file).to(be_valid_block_file)
                self.file = file
                wait_for_file_initialization_done(self.block_api, file.id, 1)

                t = tvlp_model()
                if self._block_linked:
                    t.profile.block = tvlp_block_profile_model(1, 100000000, file.id)
                if self._memory_linked:
                    t.profile.memory = tvlp_memory_profile_model(1, 100000000)
                if self._cpu_linked:
                    t.profile.cpu = tvlp_cpu_profile_model(1, 100000000)
                self._config = self.tvlp_api.create_tvlp_configuration(t)

                self._result = self.tvlp_api.start_tvlp_configuration_with_http_info(self._config.id)
                expect(self._result[1]).to(equal(201))
                self._result = self.tvlp_api.stop_tvlp_configuration_with_http_info(self._config.id)
                expect(self._result[1]).to(equal(204))

            with description('by id,'):
                with it('succeeds'):
                    result = self.tvlp_api.delete_tvlp_configuration_with_http_info(self._config.id)
                    expect(result[1]).to(equal(204))

            with description('non-existent id,'):
                with it('returns 404'):
                    expect(lambda: self.tvlp_api.delete_tvlp_configuration('foo')).to(raise_api_exception(404))

            with description('invalid id,'):
                with it('returns 400'):
                    expect(lambda: self.tvlp_api.delete_tvlp_configuration('f_oo')).to(raise_api_exception(400))


        with after.each:
            try:
                for conf in self.tvlp_api.list_tvlp_configurations():
                    self.tvlp_api.stop_tvlp_configuration(conf.id)
                    self.tvlp_api.delete_tvlp_configuration(conf.id)
            except AttributeError:
                pass

            try:
                for res in self.tvlp_api.list_tvlp_configuration_results():
                    self.tvlp_api.delete_tvlp_configuration_results(res.id)
            except AttributeError:
                pass

            try:
                for gen in self.block_api.list_block_generators():
                    if gen.running:
                        self.block_api.stop_block_generator(gen.id)
                    self.block_api.delete_block_generator(gen.id)
            except AttributeError:
                pass

            try:
                for file in self.block_api.list_block_files():
                    self.block_api.delete_block_file(file.id)
            except AttributeError:
                pass

            try:
                for res in self.block_api.list_block_generator_results():
                    self.block_api.delete_block_generator_results(res.id)
            except AttributeError:
                pass

            try:
                for gen in self.memory_api.list_memory_generators():
                    if gen.running:
                        self.memory_api.stop_memory_generator(gen.id)
                    self.memory_api.delete_memory_generator(gen.id)
            except AttributeError:
                pass

            try:
                for file in self.memory_api.list_memory_files():
                    self.memory_api.delete_memory_file(file.id)
            except AttributeError:
                pass

            try:
                for res in self.memory_api.list_memory_generator_results():
                    self.memory_api.delete_memory_generator_results(res.id)
            except AttributeError:
                pass

            try:
                for gen in self.cpu_api.list_cpu_generators():
                    if gen.running:
                        self.cpu_api.stop_cpu_generator(gen.id)
                    self.cpu_api.delete_cpu_generator(gen.id)
            except AttributeError:
                pass

            try:
                for file in self.cpu_api.list_cpu_files():
                    self.cpu_api.delete_cpu_file(file.id)
            except AttributeError:
                pass

            try:
                for res in self.cpu_api.list_cpu_generator_results():
                    self.cpu_api.delete_cpu_generator_results(res.id)
            except AttributeError:
                pass

        with after.all:
            try:
                self.process.terminate()
                self.process.wait()
            except AttributeError:
                pass
