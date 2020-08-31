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
                           generator_model,
                           file_model,
                           bulk_start_model,
                           bulk_stop_model,
                           wait_for_file_initialization_done)


from common.matcher import (be_valid_block_file,
                            be_valid_block_generator,
                            be_valid_block_generator_result,
                            has_location,
                            raise_api_exception)

# TVLP
from common.helper import (tvlp_model, tvlp_block_profile_model)

from common.matcher import (be_valid_tvlp_configuration, be_valid_tvlp_result)


CONFIG = Config(os.path.join(os.path.dirname(__file__),
                             os.environ.get('MAMBA_CONFIG', 'config.yaml')))


with description('TVLP,', 'tvlp') as self:
    with description('REST API,'):
        with before.all:
            self.service = Service(CONFIG.service())
            self.process = self.service.start()
            self.block_api = client.api.BlockGeneratorApi(self.service.client())
            self.tvlp_api = client.api.TVLPApi(self.service.client())
            if not check_modules_exists(self.service.client(), 'tvlp'):
                self.skip()
        with description('block,'):
            with before.all:
                if not check_modules_exists(self.service.client(), 'block'):
                    self.skip()

            with description('create,'):
                with description("succeeds,"):
                    with before.all:
                        t = tvlp_model()
                        t.profile.block = tvlp_block_profile_model(1, 100000000, "tmp")
                        self._config = self.tvlp_api.create_tvlp_configuration_with_http_info(t)

                    with it('code Created'):
                        expect(self._config[1]).to(equal(201))

                    with it('has valid Location header,'):
                        expect(self._config[2]).to(has_location('/tvlp/' + self._config[0].id))

                    with it('has a valid tvlp configuration'):
                        expect(self._config[0]).to(be_valid_tvlp_configuration)

                with description("no profile entries,"):
                    with it('returns 400'):
                        expect(lambda: self.tvlp_api.create_tvlp_configuration(tvlp_model())).to(raise_api_exception(400))

                with description('empty resource id,'):
                    with it('returns 400'):
                        t = tvlp_model()
                        t.profile.block = tvlp_block_profile_model(1, 100000000, "tmp")
                        t.profile.block.series[0].resource_id = None
                        expect(lambda: self.tvlp_api.create_tvlp_configuration(t)).to(raise_api_exception(400))

            with description('list,'):
                with before.each:
                    t = tvlp_model()
                    t.profile.block = tvlp_block_profile_model(1, 100000000, "tmp")
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
                        t.profile.block = tvlp_block_profile_model(1, 100000000, "tmp")
                        self._config = self.tvlp_api.create_tvlp_configuration(t)

                    with it('succeeds'):
                        expect(self.tvlp_api.get_tvlp_configuration(self._config.id)).to(be_valid_tvlp_configuration)

                with description('non-existent generator,'):
                    with it('returns 404'):
                        expect(lambda: self.tvlp_api.get_tvlp_configuration('foo')).to(raise_api_exception(404))

                with description('invalid id,'):
                    with it('returns 400'):
                        expect(lambda: self.tvlp_api.get_tvlp_configuration('f_oo')).to(raise_api_exception(400))

            with description('delete generator,'):
                with description('by existing id,'):
                    with before.each:
                        t = tvlp_model()
                        t.profile.block = tvlp_block_profile_model(1, 100000000, "tmp")
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

            with description('start generator,'):
                with before.each:
                    file = self.block_api.create_block_file(file_model(1024, '/tmp/foo'))
                    expect(file).to(be_valid_block_file)
                    self.file = file
                    wait_for_file_initialization_done(self.block_api, file.id, 1)

                    t = tvlp_model()
                    t.profile.block = tvlp_block_profile_model(1, 100000000, file.id)
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

                with description('running generator,'):
                    with it('returns 400'):
                        self.tvlp_api.start_tvlp_configuration(self._config.id)
                        expect(lambda: self.tvlp_api.start_tvlp_configuration(self._config.id)).to(raise_api_exception(400))

                with description('non-existent id,'):
                    with it('returns 404'):
                        expect(lambda: self.tvlp_api.start_tvlp_configuration('foo')).to(raise_api_exception(404))

                with description('invalid id,'):
                    with it('returns 400'):
                        expect(lambda: self.tvlp_api.start_tvlp_configuration('f_oo')).to(raise_api_exception(400))

            with description('stop running generator,'):
                with before.each:
                    file = self.block_api.create_block_file(file_model(1024, '/tmp/foo'))
                    expect(file).to(be_valid_block_file)
                    self.file = file
                    wait_for_file_initialization_done(self.block_api, file.id, 1)

                    t = tvlp_model()
                    t.profile.block = tvlp_block_profile_model(1, 100000000, file.id)
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

            with description('list generator results,'):
                with before.each:
                    file = self.block_api.create_block_file(file_model(1024, '/tmp/foo'))
                    expect(file).to(be_valid_block_file)
                    self.file = file
                    wait_for_file_initialization_done(self.block_api, file.id, 1)

                    t = tvlp_model()
                    t.profile.block = tvlp_block_profile_model(1, 100000000, file.id)
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

            with description('delete generator results,'):
                with before.each:
                    file = self.block_api.create_block_file(file_model(1024, '/tmp/foo'))
                    expect(file).to(be_valid_block_file)
                    self.file = file
                    wait_for_file_initialization_done(self.block_api, file.id, 1)

                    t = tvlp_model()
                    t.profile.block = tvlp_block_profile_model(1, 100000000, file.id)
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

        with after.all:
            try:
                self.process.terminate()
                self.process.wait()
            except AttributeError:
                pass
