import os
import time
import client.api
import client.models

from mamba import description, before, after, it
from expects import *
from expects.matchers import Matcher
from common import Config, Service
from common.helper import make_dynamic_results_config
from common.helper import check_modules_exists
from common.matcher import (raise_api_exception,
                            be_valid_memory_info,
                            be_valid_memory_generator,
                            be_valid_memory_generator_result,
                            be_valid_dynamic_results)


CONFIG = Config(os.path.join(os.path.dirname(__file__),
                os.environ.get('MAMBA_CONFIG', 'config.yaml')))


def get_dynamic_results_fields():
    fields = []
    swagger_types = client.models.MemoryGeneratorStats.swagger_types
    for (name, type) in swagger_types.items():
        if type in ['int', 'float']:
            fields.append('read.' + name)
            fields.append('write.' + name)
    return fields


def generator_model(api_client, id = ''):
    config = client.models.MemoryGeneratorConfig()
    config.buffer_size = 1024
    config.reads_per_sec = 128
    config.read_size = 8
    config.read_threads = 1
    config.writes_per_sec = 128
    config.write_size = 8
    config.write_threads = 1
    config.pattern = 'sequential'

    gen = client.models.MemoryGenerator()
    gen.running = False
    gen.config = config
    gen.id = id
    gen.init_percent_complete = 0
    return gen

def wait_for_buffer_initialization_done(api_client, generator_id, timeout):
    for i in range(timeout * 10):
        g7r = api_client.get_memory_generator(generator_id)
        if g7r.init_percent_complete == 100:
            return True
        time.sleep(.1)
    return False

class _has_json_content_type(Matcher):
    def _match(self, request):
        expect(request).to(have_key('Content-Type'))
        expect(request['Content-Type']).to(equal('application/json'))
        return True, ['is JSON content type']


class has_location(Matcher):
    def __init__(self, expected):
        self._expected = expected

    def _match(self, subject):
        expect(subject).to(have_key('Location'))
        return subject['Location'] == self._expected, []


has_json_content_type = _has_json_content_type()


with description('Memory Generator Module', 'memory') as self:
    with before.all:
        service = Service(CONFIG.service())
        self._process = service.start()
        self._api = client.api.MemoryGeneratorApi(service.client())
        if not check_modules_exists(service.client(), 'memory'):
            self.skip()

    with after.all:
        try:
            self._process.terminate()
            self._process.wait()
        except AttributeError:
            pass

    with description('/memory-info'):
        with context('GET'):
            with before.all:
                self._result = self._api.memory_info_with_http_info(
                    _return_http_data_only=False)

            with it('succeeded'):
                expect(self._result[1]).to(equal(200))

            with it('has application/json header'):
                expect(self._result[2]).to(has_json_content_type)

            with it('valid memory info'):
                expect(self._result[0]).to(be_valid_memory_info)

    with description('Memory Generators'):
        with description('/memory-generators'):
            with context('POST'):
                with shared_context('create generator'):
                    with after.all:
                        self._api.delete_memory_generator(self._result[0].id)

                    with it('created'):
                        expect(self._result[1]).to(equal(201))

                    with it('has valid Location header'):
                        expect(self._result[2]).to(has_location(
                            '/memory-generators/' + self._result[0].id))

                    with it('has Content-Type: application/json header'):
                        expect(self._result[2]).to(has_json_content_type)

                    with it('returned valid generator'):
                        expect(self._result[0]).to(be_valid_memory_generator)

                    with it('has same config'):
                        if (not self._model.id):
                            self._model.id = self._result[0].id
                        self._model.init_percent_complete = self._result[0].init_percent_complete
                        expect(self._result[0]).to(equal(self._model))

                with description('with empty ID'):
                    with before.all:
                        self._model = generator_model(self._api.api_client)
                        self._result = self._api.create_memory_generator_with_http_info(
                            self._model, _return_http_data_only=False)

                    with included_context('create generator'):
                        with it('random ID assigned'):
                            expect(self._result[0].id).not_to(be_empty)

                with description('with specified ID'):
                    with before.all:
                        self._model = generator_model(
                            self._api.api_client, id='some-specified-id')
                        self._result = self._api.create_memory_generator_with_http_info(
                            self._model, _return_http_data_only=False)

                    with included_context('create generator'):
                        pass

            with context('GET'):
                with before.all:
                    model = generator_model(self._api.api_client)
                    self._g8s = [self._api.create_memory_generator(model) for a in range(3)]
                    self._result = self._api.list_memory_generators_with_http_info(
                        _return_http_data_only=False)

                with after.all:
                    for g7r in self._g8s:
                        self._api.delete_memory_generator(g7r.id)

                with it('succeeded'):
                    expect(self._result[1]).to(equal(200))

                with it('has Content-Type: application/json header'):
                    expect(self._result[2]).to(has_json_content_type)

                with it('return list'):
                    expect(self._result[0]).not_to(be_empty)
                    for gen in self._result[0]:
                        expect(gen).to(be_valid_memory_generator)

        with description('/memory-generators/{id}'):
            with before.all:
                model = generator_model(self._api.api_client)
                g7r = self._api.create_memory_generator(model)
                expect(g7r).to(be_valid_memory_generator)
                self._g7r = g7r

            with context('GET'):
                with description('by existing ID'):
                    with before.all:
                        self._result = self._api.get_memory_generator_with_http_info(
                            self._g7r.id, _return_http_data_only=False)

                    with it('succeeded'):
                        expect(self._result[1]).to(equal(200))

                    with it('has Content-Type: application/json header'):
                        expect(self._result[2]).to(has_json_content_type)

                    with it('generator object'):
                        expect(self._result[0]).to(be_valid_memory_generator)

                with description('by non-existent ID'):
                    with it('not found (404)'):
                        expr = lambda: self._api.get_memory_generator('unknown')
                        expect(expr).to(raise_api_exception(404))

                with description('by invalid ID'):
                    with it('bad request (400)'):
                        expr = lambda: self._api.get_memory_generator('bad_id')
                        expect(expr).to(raise_api_exception(400))

            with context('DELETE'):
                with description('by existing ID'):
                    with it('removed'):
                        result = self._api.delete_memory_generator_with_http_info(
                            self._g7r.id, _return_http_data_only=False)
                        expect(result[1]).to(equal(204))

                    with it('not found'):
                        expr = lambda: self._api.get_memory_generator(self._g7r.id)
                        expect(expr).to(raise_api_exception(404))

                with description('by non-existent ID'):
                    with it('not found (404)'):
                        expr = lambda: self._api.delete_memory_generator('unknown')
                        expect(expr).to(raise_api_exception(404))

                with description('by invalid ID'):
                    with it('bad request (400)'):
                        expr = lambda: self._api.delete_memory_generator('bad_id')
                        expect(expr).to(raise_api_exception(400))

        with description('/memory-generators/{id}/start'):
            with before.all:
                model = generator_model(self._api.api_client)
                g7r = self._api.create_memory_generator(model)
                expect(g7r).to(be_valid_memory_generator)
                self._g7r = g7r
                expect(wait_for_buffer_initialization_done(self._api, g7r.id, 10)).to(be_true)

            with after.all:
                self._api.delete_memory_generator(self._g7r.id)

            with context('POST'):
                with description('by existing ID'):
                    with before.all:
                        self._result = self._api.start_memory_generator_with_http_info(
                            self._g7r.id, _return_http_data_only=False)

                    with it('is not running'):
                        expect(self._g7r.running).to(be_false)

                    with it('started'):
                        expect(self._result[1]).to(equal(201))

                    with it('has valid Location header'):
                        expect(self._result[2]).to(has_location(
                            '/memory-generator-results/' + self._result[0].id))

                    with it('has Content-Type: application/json header'):
                        expect(self._result[2]).to(has_json_content_type)

                    with it('returned valid result'):
                        expect(self._result[0]).to(be_valid_memory_generator_result)
                        expect(self._result[0].active).to(be_true)
                        expect(self._result[0].generator_id).to(equal(self._g7r.id))

                    with it('is running'):
                        g7r = self._api.get_memory_generator(self._g7r.id)
                        expect(g7r).to(be_valid_memory_generator)
                        expect(g7r.running).to(be_true)

                with description('by existing ID with Dynamic Results'):
                    with before.all:
                        self._api.stop_memory_generator(self._g7r.id)
                        dynamic = make_dynamic_results_config(
                            get_dynamic_results_fields())
                        self._result = self._api.start_memory_generator_with_http_info(
                            self._g7r.id, dynamic_results=dynamic, _return_http_data_only=False)

                    with it('is not running'):
                        expect(self._g7r.running).to(be_false)

                    with it('started'):
                        expect(self._result[1]).to(equal(201))

                    with it('has valid Location header'):
                        expect(self._result[2]).to(has_location(
                            '/memory-generator-results/' + self._result[0].id))

                    with it('has Content-Type: application/json header'):
                        expect(self._result[2]).to(has_json_content_type)

                    with it('returned valid result'):
                        expect(self._result[0]).to(be_valid_memory_generator_result)
                        expect(self._result[0].active).to(be_true)
                        expect(self._result[0].generator_id).to(equal(self._g7r.id))
                        expect(self._result[0].dynamic_results).to(be_valid_dynamic_results)

                    with it('is running'):
                        g7r = self._api.get_memory_generator(self._g7r.id)
                        expect(g7r).to(be_valid_memory_generator)
                        expect(g7r.running).to(be_true)

                with description('by non-existent ID'):
                    with it('not found (404)'):
                        expr = lambda: self._api.start_memory_generator('unknown')
                        expect(expr).to(raise_api_exception(404))

                with description('by invalid ID'):
                    with it('bad request (400)'):
                        expr = lambda: self._api.start_memory_generator('bad_id')
                        expect(expr).to(raise_api_exception(400))

        with description('/memory-generators/{id}/stop'):
            with before.all:
                model = generator_model(self._api.api_client)
                g7r = self._api.create_memory_generator(model)
                expect(g7r).to(be_valid_memory_generator)

                expect(wait_for_buffer_initialization_done(self._api, g7r.id, 10)).to(be_true)

                start_result = self._api.start_memory_generator_with_http_info(g7r.id)
                expect(start_result[1]).to(equal(201))

                g7r = self._api.get_memory_generator(g7r.id)
                expect(g7r).to(be_valid_memory_generator)
                self._g7r = g7r

            with after.all:
                self._api.delete_memory_generator(self._g7r.id)

            with context('POST'):
                with description('by existing ID'):
                    with it('is running'):
                        expect(self._g7r.running).to(be_true)

                    with it('stopped'):
                        result = self._api.stop_memory_generator_with_http_info(
                            self._g7r.id, _return_http_data_only=False)
                        expect(result[1]).to(equal(204))

                    with it('is not running'):
                        g7r = self._api.get_memory_generator(self._g7r.id)
                        expect(g7r).to(be_valid_memory_generator)
                        expect(g7r.running).to(be_false)

                with description('by non-existent ID'):
                    with it('not found (404)'):
                        expr = lambda: self._api.start_memory_generator('unknown')
                        expect(expr).to(raise_api_exception(404))

                with description('by invalid ID'):
                    with it('bad request (400)'):
                        expr = lambda: self._api.start_memory_generator('bad_id')
                        expect(expr).to(raise_api_exception(400))

        with description('/memory-generators/x/bulk-start'):
            with before.all:
                model = generator_model(self._api.api_client)
                self._g8s = [self._api.create_memory_generator(model) for a in range(3)]

                for a in range(3):
                    expect(wait_for_buffer_initialization_done(self._api, self._g8s[a].id, 10)).to(be_true)

            with after.all:
                for g7r in self._g8s:
                    self._api.delete_memory_generator(g7r.id)

            with description('POST'):
                with description('by existing IDs'):
                    with before.all:
                        request = client.models.BulkStartMemoryGeneratorsRequest(
                            [g7r.id for g7r in self._g8s])
                        self._result = self._api.bulk_start_memory_generators_with_http_info(
                            request, _return_http_data_only=False)

                    with it('is not running'):
                        for g7r in self._g8s:
                            expect(g7r.running).to(be_false)

                    with it('started'):
                        expect(self._result[1]).to(equal(200))

                    with it('has Content-Type: application/json header'):
                        expect(self._result[2]).to(has_json_content_type)

                    with it('returned valid results'):
                        for result in self._result[0]:
                            expect(result).to(be_valid_memory_generator_result)
                            expect(result.active).to(be_true)

                    with it('is running'):
                        for g7r in self._g8s:
                            result = self._api.get_memory_generator(g7r.id)
                            expect(result).to(be_valid_memory_generator)
                            expect(result.running).to(be_true)

                with description('with non-existant ID'):
                    with it('is not running'):
                        request = client.models.BulkStopMemoryGeneratorsRequest(
                            [g7r.id for g7r in self._g8s])
                        self._api.bulk_stop_memory_generators(request)
                        for g7r in self._g8s:
                            expect(g7r.running).to(be_false)

                    with it('not found (404)'):
                        request = client.models.BulkStartMemoryGeneratorsRequest(
                            [g7r.id for g7r in self._g8s] + ['unknown'])
                        expr = lambda: self._api.bulk_start_memory_generators(request)
                        expect(expr).to(raise_api_exception(404))

                    with it('is not running'):
                        for g7r in self._g8s:
                            result = self._api.get_memory_generator(g7r.id)
                            expect(result).to(be_valid_memory_generator)
                            expect(result.running).to(be_false)

                with description('with invalid ID'):
                    with it('is not running'):
                        request = client.models.BulkStopMemoryGeneratorsRequest(
                            [g7r.id for g7r in self._g8s])
                        self._api.bulk_stop_memory_generators(request)
                        for g7r in self._g8s:
                            expect(g7r.running).to(be_false)

                    with it('bad request (400)'):
                        request = client.models.BulkStartMemoryGeneratorsRequest(
                            [g7r.id for g7r in self._g8s] + ['bad_id'])
                        expr = lambda: self._api.bulk_start_memory_generators(request)
                        expect(expr).to(raise_api_exception(400))

                    with it('is not running'):
                        for g7r in self._g8s:
                            result = self._api.get_memory_generator(g7r.id)
                            expect(result).to(be_valid_memory_generator)
                            expect(result.running).to(be_false)

        with description('/memory-generators/x/bulk-stop'):
            with before.all:
                model = generator_model(
                    self._api.api_client)
                g8s = [self._api.create_memory_generator(model) for a in range(3)]

                for a in range(3):
                    expect(wait_for_buffer_initialization_done(self._api, g8s[a].id, 10)).to(be_true)
                    result = self._api.start_memory_generator_with_http_info(g8s[a].id)
                    expect(result[1]).to(equal(201))

                self._g8s = self._api.list_memory_generators()
                expect(len(self._g8s)).to(equal(3))

            with after.all:
                for g7r in self._g8s:
                    self._api.delete_memory_generator(g7r.id)

            with description('POST'):
                with description('by existing IDs'):
                    with it('is running'):
                        for g7r in self._g8s:
                            expect(g7r.running).to(be_true)

                    with it('stopped'):
                        request = client.models.BulkStopMemoryGeneratorsRequest(
                            [g7r.id for g7r in self._g8s])
                        expr = lambda: self._api.bulk_stop_memory_generators(request)
                        expect(expr).not_to(raise_api_exception(204))

                    with it('is not running'):
                        for g7r in self._g8s:
                            result = self._api.get_memory_generator(g7r.id)
                            expect(result).to(be_valid_memory_generator)
                            expect(result.running).to(be_false)

                with description('with non-existant ID'):
                    with it('is running'):
                        request = client.models.BulkStartMemoryGeneratorsRequest(
                            [g7r.id for g7r in self._g8s])
                        self._api.bulk_start_memory_generators(request)
                        for g7r in self._g8s:
                            expect(g7r.running).to(be_true)

                    with it('not found (404)'):
                        request = client.models.BulkStopMemoryGeneratorsRequest(
                            [g7r.id for g7r in self._g8s] + ['unknown'])
                        expr = lambda: self._api.bulk_stop_memory_generators(request)
                        expect(expr).to(raise_api_exception(404))

                    with it('is running'):
                        for g7r in self._g8s:
                            result = self._api.get_memory_generator(g7r.id)
                            expect(result).to(be_valid_memory_generator)
                            expect(result.running).to(be_true)

                with description('with invalid ID'):
                    with it('is running'):
                        request = client.models.BulkStartMemoryGeneratorsRequest(
                            [g7r.id for g7r in self._g8s])
                        self._api.bulk_start_memory_generators(request)
                        for g7r in self._g8s:
                            expect(g7r.running).to(be_true)

                    with it('bad request (400)'):
                        request = client.models.BulkStopMemoryGeneratorsRequest(
                            [g7r.id for g7r in self._g8s] + ['bad_id'])
                        expr = lambda: self._api.bulk_stop_memory_generators(request)
                        expect(expr).to(raise_api_exception(400))

                    with it('is running'):
                        for g7r in self._g8s:
                            result = self._api.get_memory_generator(g7r.id)
                            expect(result).to(be_valid_memory_generator)
                            expect(result.running).to(be_true)

    with description('Memory Generator Results'):
        with description('/memory-generator-results'):
            with context('GET'):
                with before.all:
                    self._result = self._api.list_memory_generator_results_with_http_info(
                        _return_http_data_only=False)

                with it('success'):
                    expect(self._result[1]).to(equal(200))

                with it('has Content-Type: application/json header'):
                    expect(self._result[2]).to(has_json_content_type)

                with it('results list'):
                    expect(self._result[0]).not_to(be_empty)
                    for result in self._result[0]:
                        expect(result).to(be_valid_memory_generator_result)

        with description('/memory-generator-results/{id}'):
            with before.all:
                rlist = self._api.list_memory_generator_results()
                expect(rlist).not_to(be_empty)
                self._result = rlist[0]

            with context('GET'):
                with description('by existing ID'):
                    with before.all:
                        self._get_result = self._api.get_memory_generator_result_with_http_info(
                            self._result.id, _return_http_data_only=False)

                    with it('success'):
                        expect(self._get_result[1]).to(equal(200))

                    with it('has Content-Type: application/json header'):
                        expect(self._get_result[2]).to(has_json_content_type)

                    with it('valid result'):
                        expect(self._get_result[0]).to(be_valid_memory_generator_result)

                with description('by non-existent ID'):
                    with it('not found (404)'):
                        expr = lambda: self._api.get_memory_generator_result('unknown')
                        expect(expr).to(raise_api_exception(404))

                with description('by invalid ID'):
                    with it('bad request (400)'):
                        expr = lambda: self._api.get_memory_generator_result('bad_id')
                        expect(expr).to(raise_api_exception(400))

            with context('DELETE'):
                with description('by existing ID'):
                    with it('removed (204)'):
                        result = self._api.delete_memory_generator_result_with_http_info(
                            self._result.id, _return_http_data_only=False)
                        expect(result[1]).to(equal(204))

                    with it('not found'):
                        expr = lambda: self._api.get_memory_generator_result(self._result.id)
                        expect(expr).to(raise_api_exception(404))

                with description('by non-existent ID'):
                    with it('not found (404)'):
                        expr = lambda: self._api.delete_memory_generator_result('unknown')
                        expect(expr).to(raise_api_exception(404))

                with description('by invalid ID'):
                    with it('bad request (400)'):
                        expr = lambda: self._api.delete_memory_generator_result('bad_id')
                        expect(expr).to(raise_api_exception(400))
        with after.each:
            try:
                for gen in self.api.list_memory_generators():
                    if gen.running:
                        self.api.stop_memory_generator(gen.id)
                    self.api.delete_memory_generator(gen.id)
            except AttributeError:
                pass

            try:
                for file in self.api.list_memory_generator_results():
                    self.api.delete_memory_generator_result(file.id)
            except AttributeError:
                pass
