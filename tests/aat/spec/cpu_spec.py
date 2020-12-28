import os
import client.api
import client.models

from mamba import description, before, after, it
from expects import *
from expects.matchers import Matcher
from common import Config, Service
from common.helper import (make_dynamic_results_config,
                           check_modules_exists,
                           get_cpu_dynamic_results_fields,
                           cpu_generator_model)
from common.matcher import (has_location,
                            has_json_content_type,
                            raise_api_exception,
                            be_valid_cpu_info,
                            be_valid_cpu_generator,
                            be_valid_cpu_generator_result,
                            be_valid_dynamic_results)


CONFIG = Config(os.path.join(os.path.dirname(__file__),
                os.environ.get('MAMBA_CONFIG', 'config.yaml')))


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

    with description('Information'):
        with description('/cpu-info'):
            with context('PUT'):
                with it('not allowed (405)'):
                    expect(lambda: self._api.api_client.call_api('/cpu-info', 'PUT')).to(
                        raise_api_exception(405, headers={'Allow': "GET"}))

            with context('GET'):
                with before.all:
                    self._result = self._api.cpu_info_with_http_info(
                        _return_http_data_only=False)

                with it('success (200)'):
                    expect(self._result[1]).to(equal(200))

                with it('has Content-Type: application/json header'):
                    expect(self._result[2]).to(has_json_content_type)

                with it('valid cpu info'):
                    expect(self._result[0]).to(be_valid_cpu_info)

    with description('CPU Generators'):
        with description('/cpu-generators'):
            with context('PUT'):
                with it('not allowed (405)'):
                    expect(lambda: self._api.api_client.call_api('/cpu-generators', 'PUT')).to(
                        raise_api_exception(405, headers={'Allow': "GET, POST"}))

            with context('POST'):
                with shared_context('create generator'):
                    with after.all:
                        self._api.delete_cpu_generator(self._result[0].id)

                    with it('created (201)'):
                        expect(self._result[1]).to(equal(201))

                    with it('has valid Location header'):
                        expect(self._result[2]).to(has_location('/cpu-generators/' + self._result[0].id))

                    with it('has Content-Type: application/json header'):
                        expect(self._result[2]).to(has_json_content_type)

                    with it('returned valid generator'):
                        expect(self._result[0]).to(be_valid_cpu_generator)

                    with it('has same config'):
                        if (not self._model.id):
                            self._model.id = self._result[0].id
                        expect(self._result[0]).to(equal(self._model))

                with description('method cores'):
                    with description('with empty ID'):
                        with before.all:
                            self._model = cpu_generator_model(self._api.api_client,
                                method='cores')
                            self._result = self._api.create_cpu_generator_with_http_info(
                                self._model, _return_http_data_only=False)

                        with included_context('create generator'):
                            with it('random ID assigned'):
                                expect(self._result[0].id).not_to(be_empty)

                    with description('with specified ID'):
                        with before.all:
                            self._model = cpu_generator_model(
                                self._api.api_client, id='some-specified-id')
                            self._result = self._api.create_cpu_generator_with_http_info(
                                self._model, _return_http_data_only=False)

                        with included_context('create generator'):
                            pass

                with description('method system'):
                    with description('with empty ID'):
                        with before.all:
                            self._model = cpu_generator_model(self._api.api_client,
                                method='system')
                            self._result = self._api.create_cpu_generator_with_http_info(
                                self._model, _return_http_data_only=False)

                        with included_context('create generator'):
                            with it('random ID assigned'):
                                expect(self._result[0].id).not_to(be_empty)

                    with description('with specified ID'):
                        with before.all:
                            self._model = cpu_generator_model(
                                self._api.api_client, id='some-specified-id')
                            self._result = self._api.create_cpu_generator_with_http_info(
                                self._model, _return_http_data_only=False)

                        with included_context('create generator'):
                            pass

            with context('GET'):
                with before.all:
                    model = cpu_generator_model(
                        self._api.api_client, running = False)
                    self._g8s = [self._api.create_cpu_generator(model)
                        for a in range(3)]
                    self._result = self._api.list_cpu_generators_with_http_info(
                        _return_http_data_only=False)

                with after.all:
                    for g7r in self._g8s:
                        self._api.delete_cpu_generator(g7r.id)

                with it('success (200)'):
                    expect(self._result[1]).to(equal(200))

                with it('has Content-Type: application/json header'):
                    expect(self._result[2]).to(has_json_content_type)

                with it('return list'):
                    expect(self._result[0]).not_to(be_empty)
                    for gen in self._result[0]:
                        expect(gen).to(be_valid_cpu_generator)

        with description('/cpu-generators/{id}'):
            with context('GET'):
                with before.all:
                    model = cpu_generator_model(
                        self._api.api_client, running = False)
                    self._g7r = self._api.create_cpu_generator(model)
                    expect(self._g7r).to(be_valid_cpu_generator)

                with after.all:
                    self._api.delete_cpu_generator(self._g7r.id)

                with description('by existing ID'):
                    with before.all:
                        self._result = self._api.get_cpu_generator_with_http_info(
                            self._g7r.id, _return_http_data_only=False)

                    with it('success (200)'):
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
                    with shared_context('delete generator'):
                        with it('deleted (204)'):
                            result = self._api.delete_cpu_generator_with_http_info(
                                self._g7r.id, _return_http_data_only=False)
                            expect(result[1]).to(equal(204))

                        with it('not found (404)'):
                            expr = lambda: self._api.get_cpu_generator(self._g7r.id)
                            expect(expr).to(raise_api_exception(404))

                    with description('not running generator'):
                        with before.all:
                            model = cpu_generator_model(
                                self._api.api_client, running = False)
                            self._g7r = self._api.create_cpu_generator(model)
                            expect(self._g7r).to(be_valid_cpu_generator)

                        with it('not running'):
                            result = self._api.get_cpu_generator(self._g7r.id)
                            expect(result.running).to(be_false)

                        with included_context('delete generator'):
                            pass

                    with description('running generator'):
                        with before.all:
                            model = cpu_generator_model(
                                self._api.api_client, running = True)
                            self._g7r = self._api.create_cpu_generator(model)
                            expect(self._g7r).to(be_valid_cpu_generator)

                        with it('running'):
                            result = self._api.get_cpu_generator(self._g7r.id)
                            expect(result.running).to(be_true)

                        with included_context('delete generator'):
                            pass

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
                model = cpu_generator_model(
                    self._api.api_client, running = False)
                self._g7r = self._api.create_cpu_generator(model)
                expect(self._g7r).to(be_valid_cpu_generator)

            with after.all:
                self._api.delete_cpu_generator(self._g7r.id)

            with context('POST'):
                with description('by existing ID'):
                    with before.all:
                        self._result = self._api.start_cpu_generator_with_http_info(
                            self._g7r.id, _return_http_data_only=False)

                    with shared_context('start generator'):
                        with it('not running'):
                            expect(self._g7r.running).to(be_false)

                        with it('created (201)'):
                            expect(self._result[1]).to(equal(201))

                        with it('has valid Location header'):
                            expect(self._result[2]).to(has_location('/cpu-generator-results/' + self._result[0].id))

                        with it('has Content-Type: application/json header'):
                            expect(self._result[2]).to(has_json_content_type)

                        with it('returned valid result'):
                            expect(self._result[0]).to(be_valid_cpu_generator_result)
                            expect(self._result[0].active).to(be_true)
                            expect(self._result[0].generator_id).to(equal(self._g7r.id))

                        with it('started'):
                            g7r = self._api.get_cpu_generator(self._g7r.id)
                            expect(g7r).to(be_valid_cpu_generator)
                            expect(g7r.running).to(be_true)

                    with included_context('start generator'):
                        pass

                        with description('already running generator'):
                            with it('bad request (400)'):
                                expr = lambda: self._api.start_cpu_generator(self._g7r.id)
                                expect(expr).to(raise_api_exception(400))

                    with description('with Dynamic Results'):
                        with before.all:
                            self._api.stop_cpu_generator(self._g7r.id)
                            dynamic = make_dynamic_results_config(
                                get_cpu_dynamic_results_fields(self._g7r.config))
                            self._result = self._api.start_cpu_generator_with_http_info(
                                self._g7r.id, dynamic_results=dynamic, _return_http_data_only=False)

                        with included_context('start generator'):
                            with it('has valid dynamic results'):
                                expect(self._result[0].dynamic_results).to(be_valid_dynamic_results)

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
                model = cpu_generator_model(
                    self._api.api_client, running=True)
                self._g7r = self._api.create_cpu_generator(model)
                expect(self._g7r).to(be_valid_cpu_generator)

            with after.all:
                self._api.delete_cpu_generator(self._g7r.id)

            with context('POST'):
                with description('by existing ID'):
                    with it('running'):
                        expect(self._g7r.running).to(be_true)

                    with it('no content (204)'):
                        result = self._api.stop_cpu_generator_with_http_info(
                            self._g7r.id, _return_http_data_only=False)
                        expect(result[1]).to(equal(204))

                    with it('stopped'):
                        g7r = self._api.get_cpu_generator(self._g7r.id)
                        expect(g7r).to(be_valid_cpu_generator)
                        expect(g7r.running).to(be_false)

                    with description('already stopped generator'):
                        with it('bad request (400)'):
                            expr = lambda: self._api.stop_cpu_generator(self._g7r.id)
                            expect(expr).to(raise_api_exception(400))

                with description('by non-existent ID'):
                    with it('not found (404)'):
                        expr = lambda: self._api.start_cpu_generator('unknown')
                        expect(expr).to(raise_api_exception(404))

                with description('by invalid ID'):
                    with it('bad request (400)'):
                        expr = lambda: self._api.start_cpu_generator('bad_id')
                        expect(expr).to(raise_api_exception(400))

    with description('CPU Generators bulk operations'):
        with description('/cpu-generators/x/bulk-create'):
            with context('PUT'):
                with it('not allowed (405)'):
                    expect(lambda: self._api.api_client.call_api('/cpu-generators/x/bulk-create', 'PUT')).to(
                        raise_api_exception(405, headers={'Allow': "POST"}))

            with description('POST'):
                with before.all:
                    self._models = [
                        cpu_generator_model(self._api.api_client, method='system'),
                        cpu_generator_model(self._api.api_client, method='cores')
                    ]
                    request = client.models.BulkCreateCpuGeneratorsRequest(self._models)
                    self._result = self._api.bulk_create_cpu_generators_with_http_info(
                        request, _return_http_data_only=False)

                with after.all:
                    for g7r in self._result[0]:
                        self._api.delete_cpu_generator(g7r.id)

                with it('created (200)'):
                    expect(self._result[1]).to(equal(200))

                with it('has Content-Type: application/json header'):
                    expect(self._result[2]).to(has_json_content_type)

                with it('returned valid generator list'):
                    for g7r in self._result[0]:
                        expect(g7r).to(be_valid_cpu_generator)

                with it('has same config'):
                    for idx in range(len(self._models)):
                        model = self._models[idx]
                        if (not model.id):
                            model.id = self._result[0][idx].id
                        expect(self._result[0][idx]).to(equal(model))

        with description('/cpu-generators/x/bulk-delete'):
            with context('PUT'):
                with it('not allowed (405)'):
                    expect(lambda: self._api.api_client.call_api('/cpu-generators/x/bulk-delete', 'PUT')).to(
                        raise_api_exception(405, headers={'Allow': "POST"}))

            with context('POST'):
                with before.all:
                    self._ids = []
                    self._model = cpu_generator_model(
                        self._api.api_client, running=False)

                with shared_context('delete generators'):
                    with before.all:
                        self._g8s = [
                            self._api.create_cpu_generator(self._model)
                                for i in range(3)]

                    with it('all exist'):
                        for g7r in self._g8s:
                            result = self._api.get_cpu_generator(g7r.id)
                            expect(result).to(be_valid_cpu_generator)

                    with it('no content (204)'):
                        request = client.models.BulkDeleteCpuGeneratorsRequest(
                            [g7r.id for g7r in self._g8s] + self._ids)
                        result = self._api.bulk_delete_cpu_generators_with_http_info(
                            request, _return_http_data_only=False)
                        expect(result[1]).to(equal(204))

                    with it('all deleted'):
                        for g7r in self._g8s:
                            result = lambda: self._api.get_cpu_generator(g7r.id)
                            expect(result).to(raise_api_exception(404))

                with description('with existing IDs'):
                    with included_context('delete generators'):
                        pass

                with description('with non-existent ID'):
                    with before.all:
                        self._ids = ['unknown']

                    with included_context('delete generators'):
                        pass

                with description('with invalid ID'):
                    with before.all:
                        self._ids = ['bad_id']

                    with included_context('delete generators'):
                        pass

        with description('/cpu-generators/x/bulk-start'):
            with context('PUT'):
                with it('not allowed (405)'):
                    expect(lambda: self._api.api_client.call_api('/cpu-generators/x/bulk-start', 'PUT')).to(
                        raise_api_exception(405, headers={'Allow': "POST"}))

            with description('POST'):
                with before.all:
                    model = cpu_generator_model(
                        self._api.api_client, running = False)
                    self._g8s = [
                        self._api.create_cpu_generator(model)
                            for i in range(3)]

                with after.all:
                    request = client.models.BulkDeleteCpuGeneratorsRequest(
                        [g7r.id for g7r in self._g8s])
                    self._api.bulk_delete_cpu_generators(request)

                with description('by existing IDs'):
                    with before.all:
                        request = client.models.BulkStartCpuGeneratorsRequest(
                            [g7r.id for g7r in self._g8s])
                        self._result = self._api.bulk_start_cpu_generators_with_http_info(
                            request, _return_http_data_only=False)

                    with it('all not running'):
                        for g7r in self._g8s:
                            expect(g7r.running).to(be_false)

                    with it('success (200)'):
                        expect(self._result[1]).to(equal(200))

                    with it('has Content-Type: application/json header'):
                        expect(self._result[2]).to(has_json_content_type)

                    with it('returned valid results'):
                        for result in self._result[0]:
                            expect(result).to(be_valid_cpu_generator_result)
                            expect(result.active).to(be_true)

                    with it('all started'):
                        for g7r in self._g8s:
                            result = self._api.get_cpu_generator(g7r.id)
                            expect(result).to(be_valid_cpu_generator)
                            expect(result.running).to(be_true)

                    with description('already running generators'):
                        with it('bad request (400)'):
                            request = client.models.BulkStartCpuGeneratorsRequest(
                                [g7r.id for g7r in self._g8s])
                            expr = lambda: self._api.bulk_start_cpu_generators(request)
                            expect(expr).to(raise_api_exception(400))

                        with it('state was not changed'):
                            for g7r in self._g8s:
                                result = self._api.get_cpu_generator(g7r.id)
                                expect(result).to(be_valid_cpu_generator)
                                expect(result.running).to(be_true)

                with description('with non-existent ID'):
                    with before.all:
                        for num, g7r in enumerate(self._g8s, start=1):
                            try:
                                if (num % 2) == 0:
                                    g7r.running = False
                                    self._api.stop_cpu_generator(g7r.id)
                                else:
                                    g7r.running = True
                                    self._api.start_cpu_generator(g7r.id)
                            except Exception:
                                pass
                        self._results_count = len(self._api.list_cpu_generator_results())

                    with it('not found (404)'):
                        request = client.models.BulkStartCpuGeneratorsRequest(
                            [g7r.id for g7r in self._g8s] + ['unknown'])
                        expr = lambda: self._api.bulk_start_cpu_generators(request)
                        expect(expr).to(raise_api_exception(404))

                    with it('state was not changed'):
                        for g7r in self._g8s:
                            result = self._api.get_cpu_generator(g7r.id)
                            expect(result).to(be_valid_cpu_generator)
                            expect(result.running).to(equal(g7r.running))

                    with it('new results was not created'):
                        results = self._api.list_cpu_generator_results()
                        expect(len(results)).to(equal(self._results_count))

                with description('with invalid ID'):
                    with before.all:
                        self._results_count = len(self._api.list_cpu_generator_results())

                    with it('bad request (400)'):
                        request = client.models.BulkStartCpuGeneratorsRequest(
                            [g7r.id for g7r in self._g8s] + ['bad_id'])
                        expr = lambda: self._api.bulk_start_cpu_generators(request)
                        expect(expr).to(raise_api_exception(400))

                    with it('state was not changed'):
                        for g7r in self._g8s:
                            result = self._api.get_cpu_generator(g7r.id)
                            expect(result).to(be_valid_cpu_generator)
                            expect(result.running).to(equal(g7r.running))

                    with it('new results was not created'):
                        results = self._api.list_cpu_generator_results()
                        expect(len(results)).to(equal(self._results_count))

        with description('/cpu-generators/x/bulk-stop'):
            with context('PUT'):
                with it('not allowed (405)'):
                    expect(lambda: self._api.api_client.call_api('/cpu-generators/x/bulk-stop', 'PUT')).to(
                        raise_api_exception(405, headers={'Allow': "POST"}))

            with description('POST'):
                with before.all:
                    self._ids = []
                    model = cpu_generator_model(self._api.api_client)
                    self._g8s = [
                        self._api.create_cpu_generator(model)
                            for i in range(3)]

                with after.all:
                    request = client.models.BulkDeleteCpuGeneratorsRequest(
                        [g7r.id for g7r in self._g8s])
                    self._api.bulk_delete_cpu_generators(request)

                with shared_context('stop generators'):
                    with before.all:
                        for g7r in self._g8s:
                            self._api.start_cpu_generator(g7r.id)

                    with it('all running'):
                        for g7r in self._g8s:
                            result = self._api.get_cpu_generator(g7r.id)
                            expect(result.running).to(be_true)

                    with it('no content (204)'):
                        request = client.models.BulkStopCpuGeneratorsRequest(
                            [g7r.id for g7r in self._g8s] + self._ids)
                        result = self._api.bulk_stop_cpu_generators_with_http_info(
                            request, _return_http_data_only=False)
                        expect(result[1]).to(equal(204))

                    with it('all stopped'):
                        for g7r in self._g8s:
                            result = self._api.get_cpu_generator(g7r.id)
                            expect(result).to(be_valid_cpu_generator)
                            expect(result.running).to(be_false)

                with description('with existing IDs'):
                    with included_context('stop generators'):
                        pass

                    with description('already stopped generators'):
                        with it('no content (204)'):
                            request = client.models.BulkStopCpuGeneratorsRequest(
                                [g7r.id for g7r in self._g8s])
                            result = self._api.bulk_stop_cpu_generators_with_http_info(
                                request, _return_http_data_only=False)
                            expect(result[1]).to(equal(204))

                with description('with non-existent ID'):
                    with before.all:
                        self._ids = ['unknown']

                    with included_context('stop generators'):
                        pass

                with description('with invalid ID'):
                    with before.all:
                        self._ids = ['bad_id']

                    with included_context('stop generators'):
                        pass

    with description('CPU Generator Results'):
        with before.all:
            model = cpu_generator_model(self._api.api_client, running = False)
            self._g7r = self._api.create_cpu_generator(model)
            expect(self._g7r).to(be_valid_cpu_generator)
            self._runs = 3;
            for i in range(self._runs):
                self._api.start_cpu_generator(self._g7r.id)
                self._api.stop_cpu_generator(self._g7r.id)

        with description('/cpu-generator-results'):
            with context('PUT'):
                with it('not allowed (405)'):
                    expect(lambda: self._api.api_client.call_api('/cpu-generator-results', 'PUT')).to(
                        raise_api_exception(405, headers={'Allow': "GET"}))

            with context('GET'):
                with before.all:
                    self._result = self._api.list_cpu_generator_results_with_http_info(
                        _return_http_data_only=False)

                with it('success (200)'):
                    expect(self._result[1]).to(equal(200))

                with it('has Content-Type: application/json header'):
                    expect(self._result[2]).to(has_json_content_type)

                with it('results list'):
                    expect(self._result[0]).not_to(be_empty)
                    expect(len(self._result[0])).to(be(self._runs))
                    for result in self._result[0]:
                        expect(result).to(be_valid_cpu_generator_result)

        with description('/cpu-generator-results/{id}'):
            with before.all:
                rlist = self._api.list_cpu_generator_results()
                expect(rlist).not_to(be_empty)
                self._result = rlist[0]

            with context('GET'):
                with description('by existing ID'):
                    with before.all:
                        self._get_result = self._api.get_cpu_generator_result_with_http_info(
                            self._result.id, _return_http_data_only=False)

                    with it('success (200)'):
                        expect(self._get_result[1]).to(equal(200))

                    with it('has Content-Type: application/json header'):
                        expect(self._get_result[2]).to(has_json_content_type)

                    with it('valid result'):
                        expect(self._get_result[0]).to(be_valid_cpu_generator_result)

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
                        with before.all:
                            self._result = self._api.start_cpu_generator(self._g7r.id)

                        with after.all:
                            self._api.stop_cpu_generator(self._g7r.id)

                        with it('exists'):
                            expect(self._result).to(be_valid_cpu_generator_result)

                        with it('active'):
                            expect(self._result.active).to(be_true)

                        with it('not deleted (400)'):
                            result = lambda: self._api.delete_cpu_generator_result(self._result.id)
                            expect(result).to(raise_api_exception(400))

                    with description('inactive result'):
                        with before.all:
                            result = self._api.start_cpu_generator(self._g7r.id)
                            self._api.stop_cpu_generator(self._g7r.id)
                            self._result = self._api.get_cpu_generator_result(result.id)

                        with it('exists'):
                            expect(self._result).to(be_valid_cpu_generator_result)

                        with it('not active'):
                            expect(self._result.active).to(be_false)

                        with it('deleted (204)'):
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

        with description('delete results with generator'):
            with it('results exist'):
                results = self._api.list_cpu_generator_results()
                expect(results).not_to(be_empty)

            with it('generator deleted'):
                result = self._api.delete_cpu_generator_with_http_info(
                    self._g7r.id, _return_http_data_only=False)
                expect(result[1]).to(equal(204))

            with it('results deleted'):
                results = self._api.list_cpu_generator_results()
                expect(results).to(be_empty)

