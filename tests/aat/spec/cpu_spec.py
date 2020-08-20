import os
import client.api
import client.models

from mamba import description, before, after, it
from expects import *
from expects.matchers import Matcher
from common import Config, Service
from common.helper import make_dynamic_results_config
from common.helper import check_modules_exists
from common.matcher import (raise_api_exception,
                            be_valid_cpu_info,
                            be_valid_cpu_generator,
                            be_valid_cpu_generator_result,
                            be_valid_dynamic_results)


CONFIG = Config(os.path.join(os.path.dirname(__file__),
                os.environ.get('MAMBA_CONFIG', 'config.yaml')))


def get_dynamic_results_fields(generator_config):
    fields = []
    for (name, type) in client.models.CpuGeneratorStats.swagger_types.items():
        if type in ['int', 'float']:
            fields.append(name)

    for i in range(len(generator_config.cores)):
        swagger_types = client.models.CpuGeneratorCoreStats.swagger_types
        for (name, type) in swagger_types.items():
            if type in ['int', 'float']:
                fields.append('cores[' + str(i) + '].' + name)

    return fields


def generator_model(api_client, running = False, id = ''):
    core_config_target = client.models.CpuGeneratorCoreConfigTargets()
    core_config_target.instruction_set = 'scalar'
    core_config_target.data_type = 'float32';
    core_config_target.weight = 1

    core_config = client.models.CpuGeneratorCoreConfig()
    core_config.utilization = 50.0
    core_config.targets = [core_config_target]

    config = client.models.CpuGeneratorConfig()
    config.cores = [core_config]

    gen = client.models.CpuGenerator()
    gen.running = running
    gen.config = config
    gen.id = id
    return gen


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


with description('CPU Generator Module', 'cpu') as self:
    with before.all:
        service = Service(CONFIG.service())
        self._process = service.start()
        self._api = client.api.CpuGeneratorApi(service.client())
        if not check_modules_exists(service.client(), 'cpu'):
            self.skip()

    with after.all:
        try:
            for gen in self._api.list_cpu_generators():
                if gen.running:
                    self._api.stop_cpu_generator(gen.id)
                self._api.delete_cpu_generator(gen.id)

            self._process.terminate()
            self._process.wait()
        except AttributeError:
            pass

    with description('/cpu-info'):
        with context('GET'):
            with before.all:
                self._result = self._api.cpu_info_with_http_info(
                    _return_http_data_only=False)

            with it('succeeded'):
                expect(self._result[1]).to(equal(200))

            with it('has application/json header'):
                expect(self._result[2]).to(has_json_content_type)

            with it('valid cpu info'):
                expect(self._result[0]).to(be_valid_cpu_info)

    with description('CPU Generators'):
        with description('/cpu-generators'):
            with context('POST'):
                with shared_context('create generator'):
                    with after.all:
                        self._api.delete_cpu_generator(self._result[0].id)

                    with it('created'):
                        expect(self._result[1]).to(equal(201))

                    with it('has valid Location header'):
                        expect(self._result[2]).to(has_location(
                            '/cpu-generators/' + self._result[0].id))

                    with it('has Content-Type: application/json header'):
                        expect(self._result[2]).to(has_json_content_type)

                    with it('returned valid generator'):
                        expect(self._result[0]).to(be_valid_cpu_generator)

                    with it('has same config'):
                        if (not self._model.id):
                            self._model.id = self._result[0].id
                        expect(self._result[0]).to(equal(self._model))

                with description('with empty ID'):
                    with before.all:
                        self._model = generator_model(self._api.api_client)
                        self._result = self._api.create_cpu_generator_with_http_info(
                            self._model, _return_http_data_only=False)

                    with included_context('create generator'):
                        with it('random ID assigned'):
                            expect(self._result[0].id).not_to(be_empty)

                with description('with specified ID'):
                    with before.all:
                        self._model = generator_model(
                            self._api.api_client, id='some-specified-id')
                        self._result = self._api.create_cpu_generator_with_http_info(
                            self._model, _return_http_data_only=False)

                    with included_context('create generator'):
                        pass

            with context('GET'):
                with before.all:
                    model = generator_model(
                        self._api.api_client, running = False)
                    self._g8s = [self._api.create_cpu_generator(model) for a in range(3)]
                    self._result = self._api.list_cpu_generators_with_http_info(
                        _return_http_data_only=False)

                with after.all:
                    for g7r in self._g8s:
                        self._api.delete_cpu_generator(g7r.id)

                with it('succeeded'):
                    expect(self._result[1]).to(equal(200))

                with it('has Content-Type: application/json header'):
                    expect(self._result[2]).to(has_json_content_type)

                with it('return list'):
                    expect(self._result[0]).not_to(be_empty)
                    for gen in self._result[0]:
                        expect(gen).to(be_valid_cpu_generator)

        with description('/cpu-generators/{id}'):
            with before.all:
                model = generator_model(
                    self._api.api_client, running = False)
                g7r = self._api.create_cpu_generator(model)
                expect(g7r).to(be_valid_cpu_generator)
                self._g7r = g7r

            with context('GET'):
                with description('by existing ID'):
                    with before.all:
                        self._result = self._api.get_cpu_generator_with_http_info(
                            self._g7r.id, _return_http_data_only=False)

                    with it('succeeded'):
                        expect(self._result[1]).to(equal(200))

                    with it('has Content-Type: application/json header'):
                        expect(self._result[2]).to(has_json_content_type)

                    with it('generator object'):
                        expect(self._result[0]).to(be_valid_cpu_generator)

                with description('by non-existent ID'):
                    with it('not found (404)'):
                        expr = lambda: self._api.get_cpu_generator('unknown')
                        expect(expr).to(raise_api_exception(404))

                with description('by invalid ID'):
                    with it('bad request (400)'):
                        expr = lambda: self._api.get_cpu_generator('bad_id')
                        expect(expr).to(raise_api_exception(400))

            with context('DELETE'):
                with description('by existing ID'):
                    with it('removed'):
                        result = self._api.delete_cpu_generator_with_http_info(
                            self._g7r.id, _return_http_data_only=False)
                        expect(result[1]).to(equal(204))

                    with it('not found'):
                        expr = lambda: self._api.get_cpu_generator(self._g7r.id)
                        expect(expr).to(raise_api_exception(404))

                with description('by non-existent ID'):
                    with it('not found (404)'):
                        expr = lambda: self._api.delete_cpu_generator('unknown')
                        expect(expr).to(raise_api_exception(404))

                with description('by invalid ID'):
                    with it('bad request (400)'):
                        expr = lambda: self._api.delete_cpu_generator('bad_id')
                        expect(expr).to(raise_api_exception(400))

        with description('/cpu-generators/{id}/start'):
            with before.all:
                model = generator_model(
                    self._api.api_client, running = False)
                g7r = self._api.create_cpu_generator(model)
                expect(g7r).to(be_valid_cpu_generator)
                self._g7r = g7r

            with after.all:
                self._api.delete_cpu_generator(self._g7r.id)

            with context('POST'):
                with description('by existing ID'):
                    with before.all:
                        self._result = self._api.start_cpu_generator_with_http_info(
                            self._g7r.id, _return_http_data_only=False)

                    with it('is not running'):
                        expect(self._g7r.running).to(be_false)

                    with it('started'):
                        expect(self._result[1]).to(equal(201))

                    with it('has valid Location header'):
                        expect(self._result[2]).to(has_location(
                            '/cpu-generator-results/' + self._result[0].id))

                    with it('has Content-Type: application/json header'):
                        expect(self._result[2]).to(has_json_content_type)

                    with it('returned valid result'):
                        expect(self._result[0]).to(be_valid_cpu_generator_result)

                    with it('is running'):
                        g7r = self._api.get_cpu_generator(self._g7r.id)
                        expect(g7r).to(be_valid_cpu_generator)
                        expect(g7r.running).to(be_true)

                with description('by existing ID with Dynamic Results'):
                    with before.all:
                        self._api.stop_cpu_generator(self._g7r.id)
                        dynamic = make_dynamic_results_config(
                            get_dynamic_results_fields(self._g7r.config))
                        self._result = self._api.start_cpu_generator_with_http_info(
                            self._g7r.id, dynamic_results=dynamic, _return_http_data_only=False)

                    with it('is not running'):
                        expect(self._g7r.running).to(be_false)

                    with it('started'):
                        expect(self._result[1]).to(equal(201))

                    with it('has valid Location header'):
                        expect(self._result[2]).to(has_location(
                            '/cpu-generator-results/' + self._result[0].id))

                    with it('has Content-Type: application/json header'):
                        expect(self._result[2]).to(has_json_content_type)

                    with it('returned valid result'):
                        expect(self._result[0]).to(be_valid_cpu_generator_result)
                        expect(self._result[0].active).to(be_true)
                        expect(self._result[0].generator_id).to(equal(self._g7r.id))
                        expect(self._result[0].dynamic_results).to(be_valid_dynamic_results)

                    with it('is running'):
                        g7r = self._api.get_cpu_generator(self._g7r.id)
                        expect(g7r).to(be_valid_cpu_generator)
                        expect(g7r.running).to(be_true)

                with description('by non-existent ID'):
                    with it('not found (404)'):
                        expr = lambda: self._api.start_cpu_generator('unknown')
                        expect(expr).to(raise_api_exception(404))

                with description('by invalid ID'):
                    with it('bad request (400)'):
                        expr = lambda: self._api.start_cpu_generator('bad_id')
                        expect(expr).to(raise_api_exception(400))

        with description('/cpu-generators/{id}/stop'):
            with before.all:
                model = generator_model(
                    self._api.api_client, running = True)
                g7r = self._api.create_cpu_generator(model)
                expect(g7r).to(be_valid_cpu_generator)
                self._g7r = g7r

            with after.all:
                self._api.delete_cpu_generator(self._g7r.id)

            with context('POST'):
                with description('by existing ID'):
                    with it('is running'):
                        expect(self._g7r.running).to(be_true)

                    with it('stopped'):
                        result = self._api.stop_cpu_generator_with_http_info(
                            self._g7r.id, _return_http_data_only=False)
                        expect(result[1]).to(equal(204))

                    with it('is not running'):
                        g7r = self._api.get_cpu_generator(self._g7r.id)
                        expect(g7r).to(be_valid_cpu_generator)
                        expect(g7r.running).to(be_false)

                with description('by non-existent ID'):
                    with it('not found (404)'):
                        expr = lambda: self._api.start_cpu_generator('unknown')
                        expect(expr).to(raise_api_exception(404))

                with description('by invalid ID'):
                    with it('bad request (400)'):
                        expr = lambda: self._api.start_cpu_generator('bad_id')
                        expect(expr).to(raise_api_exception(400))

        with description('/cpu-generators/x/bulk-start'):
            with before.all:
                model = generator_model(
                    self._api.api_client, running = False)
                self._g8s = [self._api.create_cpu_generator(model) for a in range(3)]

            with after.all:
                for g7r in self._g8s:
                    self._api.delete_cpu_generator(g7r.id)

            with description('POST'):
                with description('by existing IDs'):
                    with before.all:
                        request = client.models.BulkStartCpuGeneratorsRequest(
                            [g7r.id for g7r in self._g8s])
                        self._result = self._api.bulk_start_cpu_generators_with_http_info(
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
                            expect(result).to(be_valid_cpu_generator_result)

                    with it('is running'):
                        for g7r in self._g8s:
                            result = self._api.get_cpu_generator(g7r.id)
                            expect(result).to(be_valid_cpu_generator)
                            expect(result.running).to(be_true)

                with description('with non-existant ID'):
                    with it('is not running'):
                        request = client.models.BulkStopCpuGeneratorsRequest(
                            [g7r.id for g7r in self._g8s])
                        self._api.bulk_stop_cpu_generators(request)
                        for g7r in self._g8s:
                            expect(g7r.running).to(be_false)

                    with it('not found (404)'):
                        request = client.models.BulkStartCpuGeneratorsRequest(
                            [g7r.id for g7r in self._g8s] + ['unknown'])
                        expr = lambda: self._api.bulk_start_cpu_generators(request)
                        expect(expr).to(raise_api_exception(404))

                    with it('is not running'):
                        for g7r in self._g8s:
                            result = self._api.get_cpu_generator(g7r.id)
                            expect(result).to(be_valid_cpu_generator)
                            expect(result.running).to(be_false)

                with description('with invalid ID'):
                    with it('is not running'):
                        request = client.models.BulkStopCpuGeneratorsRequest(
                            [g7r.id for g7r in self._g8s])
                        self._api.bulk_stop_cpu_generators(request)
                        for g7r in self._g8s:
                            expect(g7r.running).to(be_false)

                    with it('bad request (400)'):
                        request = client.models.BulkStartCpuGeneratorsRequest(
                            [g7r.id for g7r in self._g8s] + ['bad_id'])
                        expr = lambda: self._api.bulk_start_cpu_generators(request)
                        expect(expr).to(raise_api_exception(400))

                    with it('is not running'):
                        for g7r in self._g8s:
                            result = self._api.get_cpu_generator(g7r.id)
                            expect(result).to(be_valid_cpu_generator)
                            expect(result.running).to(be_false)

        with description('/cpu-generators/x/bulk-stop'):
            with before.all:
                model = generator_model(
                    self._api.api_client, running = True)
                self._g8s = [self._api.create_cpu_generator(model) for a in range(3)]

            with after.all:
                for g7r in self._g8s:
                    self._api.delete_cpu_generator(g7r.id)

            with description('POST'):
                with description('by existing IDs'):
                    with it('is running'):
                        for g7r in self._g8s:
                            expect(g7r.running).to(be_true)

                    with it('stopped'):
                        request = client.models.BulkStopCpuGeneratorsRequest(
                            [g7r.id for g7r in self._g8s])
                        result = self._api.bulk_stop_cpu_generators_with_http_info(
                          request, _return_http_data_only=False)
                        expect(result[1]).to(equal(204))

                    with it('is not running'):
                        for g7r in self._g8s:
                            result = self._api.get_cpu_generator(g7r.id)
                            expect(result).to(be_valid_cpu_generator)
                            expect(result.running).to(be_false)

                with description('with non-existant ID'):
                    with it('is running'):
                        request = client.models.BulkStartCpuGeneratorsRequest(
                            [g7r.id for g7r in self._g8s])
                        self._api.bulk_start_cpu_generators(request)
                        for g7r in self._g8s:
                            expect(g7r.running).to(be_true)

                    with it('not found (404)'):
                        request = client.models.BulkStopCpuGeneratorsRequest(
                            [g7r.id for g7r in self._g8s] + ['unknown'])
                        expr = lambda: self._api.bulk_stop_cpu_generators(request)
                        expect(expr).to(raise_api_exception(404))

                    with it('is running'):
                        for g7r in self._g8s:
                            result = self._api.get_cpu_generator(g7r.id)
                            expect(result).to(be_valid_cpu_generator)
                            expect(result.running).to(be_true)

                with description('with invalid ID'):
                    with it('is running'):
                        request = client.models.BulkStartCpuGeneratorsRequest(
                            [g7r.id for g7r in self._g8s])
                        self._api.bulk_start_cpu_generators(request)
                        for g7r in self._g8s:
                            expect(g7r.running).to(be_true)

                    with it('bad request (400)'):
                        request = client.models.BulkStopCpuGeneratorsRequest(
                            [g7r.id for g7r in self._g8s] + ['bad_id'])
                        expr = lambda: self._api.bulk_stop_cpu_generators(request)
                        expect(expr).to(raise_api_exception(400))

                    with it('is running'):
                        for g7r in self._g8s:
                            result = self._api.get_cpu_generator(g7r.id)
                            expect(result).to(be_valid_cpu_generator)
                            expect(result.running).to(be_true)

    with description('CPU Generator Results'):
        with before.all:
            model = generator_model(self._api.api_client, running = False)
            g7r = self._api.create_cpu_generator(model)
            expect(g7r).to(be_valid_cpu_generator)
            result = self._api.start_cpu_generator(g7r.id)
            self._g7r = g7r
            self._result = result

        with after.all:
            self._api.delete_cpu_generator(self._g7r.id)

        with description('/cpu-generator-results'):
            with context('GET'):
                with it('results list'):
                    rlist = self._api.list_cpu_generator_results()
                    expect(rlist).not_to(be_empty)
                    for result in rlist:
                        expect(result).to(be_valid_cpu_generator_result)

        with description('/cpu-generator-results/{id}'):
            with context('GET'):
                with description('by existing ID'):
                    with it('success'):
                        result = self._api.get_cpu_generator_result(self._result.id)
                        expect(result).to(be_valid_cpu_generator_result)

                with description('by non-existent ID'):
                    with it('not found (404)'):
                        expr = lambda: self._api.get_cpu_generator_result('unknown')
                        expect(expr).to(raise_api_exception(404))

                with description('by invalid ID'):
                    with it('bad request (400)'):
                        expr = lambda: self._api.get_cpu_generator_result('bad_id')
                        expect(expr).to(raise_api_exception(400))

            with context('DELETE'):
                with description('by existing ID'):
                    with description('active result'):
                        with it('exists'):
                            result = self._api.get_cpu_generator_result(self._result.id)
                            expect(result).to(be_valid_cpu_generator_result)

                        with it('active'):
                            result = self._api.get_cpu_generator_result(self._result.id)
                            expect(result).to(be_valid_cpu_generator_result)
                            expect(result.active).to(be_true)

                        with it('not removed (400)'):
                            result = lambda: self._api.delete_cpu_generator_result(self._result.id)
                            expect(result).to(raise_api_exception(400))

                    with description('inactive result'):
                        with it('exists'):
                            result = self._api.get_cpu_generator_result(self._result.id)
                            expect(result).to(be_valid_cpu_generator_result)

                        with it('not active'):
                            self._api.stop_cpu_generator(self._g7r.id)
                            result = self._api.get_cpu_generator_result(self._result.id)
                            expect(result).to(be_valid_cpu_generator_result)
                            expect(result.active).to(be_false)

                        with it('removed (204)'):
                            result = self._api.delete_cpu_generator_result_with_http_info(
                                self._result.id, _return_http_data_only=False)
                            expect(result[1]).to(equal(204))

                        with it('not found (404)'):
                            expr = lambda: self._api.get_cpu_generator_result(self._result.id)
                            expect(expr).to(raise_api_exception(404))

                with description('by non-existent ID'):
                    with it('not found (404)'):
                        expr = lambda: self._api.delete_cpu_generator_result('unknown')
                        expect(expr).to(raise_api_exception(404))

                with description('by invalid ID'):
                    with it('bad request (400)'):
                        expr = lambda: self._api.delete_cpu_generator_result('bad_id')
                        expect(expr).to(raise_api_exception(400))
