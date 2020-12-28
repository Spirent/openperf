import os
import time
import client.api
import client.models

from mamba import description, before, after
from expects import *
from expects.matchers import Matcher
from common import Config, Service
from common.helper import (make_dynamic_results_config,
                           check_modules_exists,
                           get_block_dynamic_results_fields,
                           block_generator_model,
                           file_model,
                           wait_for_file_initialization_done)
from common.matcher import (be_valid_block_device,
                            be_valid_block_file,
                            be_valid_block_generator,
                            be_valid_block_generator_result,
                            has_location,
                            has_json_content_type,
                            raise_api_exception,
                            be_valid_dynamic_results)


CONFIG = Config(os.path.join(os.path.dirname(__file__),
                             os.environ.get('MAMBA_CONFIG', 'config.yaml')))


with description('Block Generator Module', 'block') as self:
    with before.all:
        service = Service(CONFIG.service())
        self.process = service.start()
        self._api = client.api.BlockGeneratorApi(service.client())
        if not check_modules_exists(service.client(), 'block'):
            self.skip()

    with after.all:
        try:
            for gen in self._api.list_block_generators():
                if gen.running:
                    self._api.stop_block_generator(gen.id)
                self._api.delete_block_generator(gen.id)
        except AttributeError:
            pass

        try:
            for file in self._api.list_block_files():
                self._api.delete_block_file(file.id)
        except AttributeError:
            pass

        try:
            self.process.terminate()
            self.process.wait()
        except AttributeError:
            pass

    with description('Block Devices'):
        with description('/block-devices'):
            with context('PUT'):
                with it('not allowed (405)'):
                    expect(lambda: self._api.api_client.call_api('/block-devices', 'PUT')).to(
                        raise_api_exception(405, headers={'Allow': "GET"}))

            with context('GET'):
                with before.all:
                    self._result = self._api.list_block_devices_with_http_info(
                        _return_http_data_only=False)

                with it('success (200)'):
                    expect(self._result[1]).to(equal(200))

                with it('has Content-Type: application/json header'):
                    expect(self._result[2]).to(has_json_content_type)

                with it('valid block device list'):
                    expect(self._result[0]).not_to(be_empty)
                    for dev in self._result[0]:
                        expect(dev).to(be_valid_block_device)

        with description('/block-devices/{id}'):
            with context('GET'):
                with description('by existing ID'):
                    with before.all:
                        devices = self._api.list_block_devices()
                        self._result = self._api.get_block_device_with_http_info(
                            devices[0].id, _return_http_data_only=False)

                    with it('success (200)'):
                        expect(self._result[1]).to(equal(200))

                    with it('has Content-Type: application/json header'):
                        expect(self._result[2]).to(has_json_content_type)

                    with it('valid block device'):
                        expect(self._result[0]).to(be_valid_block_device)

                with description('by non-existent ID'):
                    with it('returns 404'):
                        expr = lambda: self._api.get_block_device('unknown')
                        expect(expr).to(raise_api_exception(404))

                with description('invalid id,'):
                    with it('returns 404'):
                        expr = lambda: self._api.get_block_device('bad_id')
                        expect(expr).to(raise_api_exception(404))

    with description('Block Files'):
        with description('/block-files'):
            with context('PUT'):
                with it('returns 405'):
                    expect(lambda: self._api.api_client.call_api('/block-files', 'PUT')).to(
                        raise_api_exception(405, headers={'Allow': "GET, POST"}))

            with context('POST'):
                with shared_context('create block file'):
                    with before.all:
                        self._result = self._api.create_block_file_with_http_info(
                            self._model, _return_http_data_only=False)

                    with after.all:
                        self._api.delete_block_file(self._result[0].id)

                    with it('created (201)'):
                        expect(self._result[1]).to(equal(201))

                    with it('has valid Location header'):
                        expect(self._result[2]).to(has_location('/block-files/' + self._result[0].id))

                    with it('has Content-Type: application/json header'):
                        expect(self._result[2]).to(has_json_content_type)

                    with it('valid block file'):
                        expect(self._result[0]).to(be_valid_block_file)

                    with it('has same config'):
                        if (not self._model.id):
                            self._model.id = self._result[0].id
                        self._model.state = self._result[0].state
                        self._model.init_percent_complete = self._result[0].init_percent_complete
                        expect(self._result[0]).to(equal(self._model))

                with description('with empty ID'):
                    with before.all:
                        self._model = file_model(1024, '/tmp/foo')

                    with included_context('create block file'):
                        with it('random ID assigned'):
                            expect(self._result[0].id).not_to(be_empty)

                with description('with specified ID'):
                    with before.all:
                        self._model = file_model(1024, '/tmp/foo', id='some-id')

                    with included_context('create block file'):
                        pass

                with description('invalid path'):
                    with it('returns 400'):
                        expr = lambda: self._api.create_block_file(
                            file_model(1024, '/tmp/foo/foo'))
                        expect(expr).to(raise_api_exception(400))

            with context('GET'):
                with before.all:
                    self._block_files = [
                        self._api.create_block_file(file_model(1024, '/tmp/foo' + str(num)))
                            for num in range(3)]
                    self._result = self._api.list_block_files_with_http_info(
                        _return_http_data_only=False)

                with after.all:
                    for block_file in self._block_files:
                        self._api.delete_block_file(block_file.id)

                with it('success (200)'):
                    expect(self._result[1]).to(equal(200))

                with it('has Content-Type: application/json header'):
                    expect(self._result[2]).to(has_json_content_type)

                with it('return list'):
                    expect(self._result[0]).not_to(be_empty)
                    expect(len(self._result[0])).to(equal(len(self._block_files)))
                    for block_file in self._result[0]:
                        expect(block_file).to(be_valid_block_file)

        with description('/block-files/{id}'):
            with before.all:
                self._file = self._api.create_block_file(
                    file_model(1024, '/tmp/foo'))
                expect(self._file).to(be_valid_block_file)

            with description('GET'):
                with description('by existing ID'):
                    with before.all:
                        self._result = self._api.get_block_file_with_http_info(
                            self._file.id, _return_http_data_only=False)

                    with it('success (200)'):
                        expect(self._result[1]).to(equal(200))

                    with it('has Content-Type: application/json header'):
                        expect(self._result[2]).to(has_json_content_type)

                    with it('valid block file'):
                        expect(self._result[0]).to(be_valid_block_file)

                with description('by non-existent ID'):
                    with it('not found (404)'):
                        expr = lambda: self._api.get_block_file('unknown')
                        expect(expr).to(raise_api_exception(404))

                with description('by invalid ID'):
                    with it('bad request (400)'):
                        expr = lambda: self._api.get_block_file('bad_id')
                        expect(expr).to(raise_api_exception(400))

            with description('DELETE'):
                with description('by existing ID'):
                    with it('deleted (204)'):
                        result = self._api.delete_block_file_with_http_info(
                            self._file.id, _return_http_data_only=False)
                        expect(result[1]).to(equal(204))

                with description('by non-existent ID'):
                    with it('not found (404)'):
                        expr = lambda: self._api.delete_block_file('unknown')
                        expect(expr).to(raise_api_exception(404))

                with description('by invalid ID'):
                    with it('bad request (400)'):
                        expr = lambda: self._api.delete_block_file('bad_id')
                        expect(expr).to(raise_api_exception(400))

    with description('Block Files bulk operations'):
        with description('/block-files/x/bulk-create'):
            with context('PUT'):
                with it('not allowed (405)'):
                    expect(lambda: self._api.api_client.call_api('/block-files/x/bulk-create', 'PUT')).to(
                        raise_api_exception(405, headers={'Allow': "POST"}))

            with context('POST'):
                with description("with empty ID"):
                    with before.all:
                        self._models = [
                            file_model(1024, '/tmp/foo' + str(num), id='foo' + str(num))
                                for num in range(3)
                        ]
                        self._result = self._api.bulk_create_block_files_with_http_info(
                            client.models.BulkCreateBlockFilesRequest(self._models))

                    with after.all:
                        for block_file in self._models:
                            self._api.delete_block_file(block_file.id)

                    with it('success (200)'):
                        expect(self._result[1]).to(equal(200))

                    with it('has Content-Type: application/json header'):
                        expect(self._result[2]).to(has_json_content_type)

                    with it('returned valid block files'):
                        expect(self._result[0]).not_to(be_empty)
                        expect(len(self._result[0])).to(equal(len(self._models)))
                        for file in self._result[0]:
                            expect(file).to(be_valid_block_file)

                    with it('has same config'):
                        for idx in range(len(self._models)):
                            model = self._models[idx]
                            model.state = self._result[0][idx].state
                            if (not model.id):
                                model.id = self._result[0][idx].id
                            expect(self._result[0][idx]).to(equal(model))

                with description('with invalid ID'):
                    with it('bad request (400)'):
                        files = [
                            file_model(1024, '/tmp/foo1'),
                            file_model(1024, '/tmp/foo', id='bad_id')
                        ]
                        expr = lambda: self._api.bulk_create_block_files(
                            client.models.BulkCreateBlockFilesRequest(files))
                        expect(expr).to(raise_api_exception(400))

                    with it('nothing created'):
                        expect(self._api.list_block_files()).to(be_empty)

                with description('with invalid path'):
                    with it('bad request (400)'):
                        files = [
                            file_model(1024, '/tmp/foo1'),
                            file_model(1024, '/tmp/foo/foo')
                        ]
                        expr = lambda: self._api.bulk_create_block_files(
                            client.models.BulkCreateBlockFilesRequest(files))
                        expect(expr).to(raise_api_exception(400))

                    with it('nothing created'):
                        expect(self._api.list_block_files()).to(be_empty)

        with description('/block-files/x/bulk-delete'):
            with context('PUT'):
                with it('not allowed (405)'):
                    expect(lambda: self._api.api_client.call_api('/block-files/x/bulk-delete', 'PUT')).to(
                        raise_api_exception(405, headers={'Allow': "POST"}))

            with context('POST'):
                with before.all:
                    self._ids = []
                    self._models = [
                        file_model(1024, '/tmp/foo' + str(num), id='foo' + str(num))
                            for num in range(3)
                    ]

                with shared_context('delete files'):
                    with before.all:
                        self._api.bulk_create_block_files(
                            client.models.BulkCreateBlockFilesRequest(self._models))

                    with it('all exist'):
                        for block_file in self._models:
                            result = self._api.get_block_file(block_file.id)
                            expect(result).to(be_valid_block_file)

                    with it('no content (204)'):
                        self._api.bulk_delete_block_files(
                            client.models.BulkDeleteBlockFilesRequest(
                                [block_file.id for block_file in self._models] + self._ids))

                    with it('all deleted'):
                        for block_file in self._models:
                            result = lambda: self._api.get_block_file(block_file.id)
                            expect(result).to(raise_api_exception(404))
                        expect(self._api.list_block_files()).to(be_empty)

                with description('with existing IDs'):
                    with included_context('delete files'):
                        pass

                with description('with non-existent ID'):
                    with before.all:
                        self._ids = ['unknown']

                    with included_context('delete files'):
                        pass

                with description('with invalid ID'):
                    with before.all:
                        self._ids = ['bad_id']

                    with included_context('delete files'):
                        pass

    with description('Block Generators'):
        with description('/block-generators'):
            with context('PUT'):
                with it('returns 405'):
                    expect(lambda: self._api.api_client.call_api('/block-generators', 'PUT')).to(
                        raise_api_exception(405, headers={'Allow': "GET, POST"}))

            with context('POST'):
                with before.all:
                    self._file = self._api.create_block_file(
                        file_model(1024, '/tmp/foo'))
                    expect(self._file).to(be_valid_block_file)
                    wait_for_file_initialization_done(self._api, self._file.id, 1)

                with after.all:
                    self._api.delete_block_file(self._file.id)

                with shared_context('create generator'):
                    with before.all:
                        self._result = self._api.create_block_generator_with_http_info(
                            self._model, _return_http_data_only=False)

                    with after.all:
                        self._api.delete_block_generator(self._result[0].id)

                    with it('created (201)'):
                        expect(self._result[1]).to(equal(201))

                    with it('has valid Location header'):
                        expect(self._result[2]).to(has_location('/block-generators/' + self._result[0].id))

                    with it('has Content-Type: application/json header'):
                        expect(self._result[2]).to(has_json_content_type)

                    with it('returned valid generator'):
                        expect(self._result[0]).to(be_valid_block_generator)

                    with it('has same config'):
                        if (not self._model.id):
                            self._model.id = self._result[0].id
                        expect(self._result[0]).to(equal(self._model))

                with description("witout ratio"):
                    with before.all:
                        self._model = block_generator_model(self._file.id)

                    with included_context('create generator'):
                        with it('ratio is not set'):
                            expect(self._result[0].config.ratio).to(be_none)

                with description("with ratio"):
                    with before.all:
                        self._model = block_generator_model(self._file.id)
                        self._model.config.ratio = client.models.BlockGeneratorReadWriteRatio()
                        self._model.config.ratio.reads = 1
                        self._model.config.ratio.writes = 1

                    with included_context('create generator'):
                        with it('ratio configured'):
                            expect(self._result[0].config.ratio).not_to(be_none)
                            expect(self._result[0].config.ratio.reads).to(be(1))
                            expect(self._result[0].config.ratio.writes).to(be(1))

                with description('with invalid ID'):
                    with it('bad request (400)'):
                        model = block_generator_model(self._file.id, id='bad_id')
                        expr = lambda: self._api.create_block_generator(model)
                        expect(expr).to(raise_api_exception(400))

                with description('with invalid pattern'):
                    with it('bad request (400)'):
                        model = block_generator_model()
                        model.config.pattern = 'foo'
                        expr = lambda: self._api.create_block_generator(model)
                        expect(expr).to(raise_api_exception(400))

                with description('with invalid resource ID'):
                    with it('bad request (400)'):
                        model = block_generator_model(resource_id='bad_id')
                        expr = lambda: self._api.create_block_generator(model)
                        expect(expr).to(raise_api_exception(400))

                with description('with non-existent resource ID'):
                    with it('bad request (400)'):
                        model = block_generator_model(resource_id='unknown')
                        expr = lambda: self._api.create_block_generator(model)
                        expect(expr).to(raise_api_exception(400))

            with context('GET'):
                with before.all:
                    self._file = self._api.create_block_file(file_model(1024, '/tmp/foo'))
                    expect(self._file).to(be_valid_block_file)
                    wait_for_file_initialization_done(self._api, self._file.id, 1)

                    model = block_generator_model(self._file.id)
                    self._g8s = [
                        self._api.create_block_generator(model)
                            for i in range(3)]
                    self._result = self._api.list_block_generators_with_http_info(
                        _return_http_data_only=False)

                with after.all:
                    for g7r in self._g8s:
                        self._api.delete_block_generator(g7r.id)
                    self._api.delete_block_file(self._file.id)

                with it('success (200)'):
                    expect(self._result[1]).to(equal(200))

                with it('has Content-Type: application/json header'):
                    expect(self._result[2]).to(has_json_content_type)

                with it('return valid list'):
                    expect(self._result[0]).not_to(be_empty)
                    expect(len(self._result[0])).to(equal(len(self._g8s)))
                    for gen in self._result[0]:
                        expect(gen).to(be_valid_block_generator)

        with description('/block-generators/{id}'):
            with before.all:
                self._file = self._api.create_block_file(
                    file_model(1024, '/tmp/foo'))
                expect(self._file).to(be_valid_block_file)
                wait_for_file_initialization_done(self._api, self._file.id, 1)

            with after.all:
                self._api.delete_block_file(self._file.id)

            with context('GET'):
                with before.all:
                    model = block_generator_model(self._file.id)
                    self._g7r = self._api.create_block_generator(model)
                    expect(self._g7r).to(be_valid_block_generator)

                with after.all:
                    self._api.delete_block_generator(self._g7r.id)

                with description('by existing ID'):
                    with before.all:
                        self._result = self._api.get_block_generator_with_http_info(
                            self._g7r.id, _return_http_data_only=False)

                    with it('success (200)'):
                        expect(self._result[1]).to(equal(200))

                    with it('has Content-Type: application/json header'):
                        expect(self._result[2]).to(has_json_content_type)

                    with it('generator object'):
                        expect(self._result[0]).to(be_valid_block_generator)

                with description('by non-existent ID'):
                    with it('not found (404)'):
                        expr = lambda: self._api.get_block_generator('unknown')
                        expect(expr).to(raise_api_exception(404))

                with description('by invalid ID'):
                    with it('bad request (400)'):
                        expr = lambda: self._api.get_block_generator('bad_id')
                        expect(expr).to(raise_api_exception(400))

            with context('DELETE'):
                with description('by existing ID'):
                    with shared_context('delete generator'):
                        with it('deleted (204)'):
                            result = self._api.delete_block_generator_with_http_info(
                                self._g7r.id, _return_http_data_only=False)
                            expect(result[1]).to(equal(204))

                        with it('not found (404)'):
                            expr = lambda: self._api.get_block_generator(self._g7r.id)
                            expect(expr).to(raise_api_exception(404))

                    with description('not running generator'):
                        with before.all:
                            model = block_generator_model(
                                self._file.id, running=False)
                            self._g7r = self._api.create_block_generator(model)
                            expect(self._g7r).to(be_valid_block_generator)

                        with it('not running'):
                            result = self._api.get_block_generator(self._g7r.id)
                            expect(result.running).to(be_false)

                        with included_context('delete generator'):
                            pass

                    with description('running generator'):
                        with before.all:
                            model = block_generator_model(
                                self._file.id, running=True)
                            self._g7r = self._api.create_block_generator(model)
                            expect(self._g7r).to(be_valid_block_generator)

                        with it('running'):
                            result = self._api.get_block_generator(self._g7r.id)
                            expect(result.running).to(be_true)

                        with included_context('delete generator'):
                            pass

                with description('by non-existent ID'):
                    with it('not found (404)'):
                        expr = lambda: self._api.delete_block_generator('unknown')
                        expect(expr).to(raise_api_exception(404))

                with description('by invalid ID'):
                    with it('bad request (400)'):
                        expr = lambda: self._api.delete_block_generator("bad_id")
                        expect(expr).to(raise_api_exception(400))

        with description('/block-generators/{id}/start'):
            with before.all:
                self._file = self._api.create_block_file(
                    file_model(1024, '/tmp/foo'))
                expect(self._file).to(be_valid_block_file)
                wait_for_file_initialization_done(self._api, self._file.id, 1)

                model = block_generator_model(self._file.id)
                self._g7r = self._api.create_block_generator(model)
                expect(self._g7r).to(be_valid_block_generator)

            with after.all:
                self._api.delete_block_generator(self._g7r.id)
                self._api.delete_block_file(self._file.id)

            with context('POST'):
                with description('by existing ID'):
                    with before.all:
                        self._result = self._api.start_block_generator_with_http_info(
                            self._g7r.id, _return_http_data_only=False)

                    with shared_context('start generator'):
                        with it('is not running'):
                            expect(self._g7r.running).to(be_false)

                        with it('started (201)'):
                            expect(self._result[1]).to(equal(201))

                        with it('has valid Location header'):
                            expect(self._result[2]).to(has_location('/block-generator-results/' + self._result[0].id))

                        with it('has Content-Type: application/json header'):
                            expect(self._result[2]).to(has_json_content_type)

                        with it('returned valid result'):
                            expect(self._result[0]).to(be_valid_block_generator_result)
                            expect(self._result[0].active).to(be_true)
                            expect(self._result[0].generator_id).to(equal(self._g7r.id))

                        with it('is running'):
                            g7r = self._api.get_block_generator(self._g7r.id)
                            expect(g7r).to(be_valid_block_generator)
                            expect(g7r.running).to(be_true)

                    with included_context('start generator'):
                        pass

                        with description('already running generator'):
                            with it('bad request (400)'):
                                expr = lambda: self._api.start_block_generator(self._g7r.id)
                                expect(expr).to(raise_api_exception(400))

                    with description('with Dynamic Results'):
                        with before.all:
                            self._api.stop_block_generator(self._g7r.id)
                            dynamic = make_dynamic_results_config(
                                get_block_dynamic_results_fields())
                            self._result = self._api.start_block_generator_with_http_info(
                                self._g7r.id, dynamic_results=dynamic, _return_http_data_only=False)

                        with included_context('start generator'):
                            with it('has valid dynamic results'):
                                expect(self._result[0].dynamic_results).to(be_valid_dynamic_results)

                with description('by non-existent ID'):
                    with it('not found (404)'):
                        expr = lambda: self._api.start_block_generator('unknown')
                        expect(expr).to(raise_api_exception(404))

                with description('by invalid ID'):
                    with it('bad request (400)'):
                        expr = lambda: self._api.start_block_generator('bad_id')
                        expect(expr).to(raise_api_exception(400))

        with description('/block-generators/{id}/stop'):
            with before.all:
                self._file = self._api.create_block_file(
                    file_model(1024, '/tmp/foo'))
                expect(self._file).to(be_valid_block_file)
                wait_for_file_initialization_done(self._api, self._file.id, 1)

                self._g7r = self._api.create_block_generator(
                    block_generator_model(self._file.id, running=True))
                expect(self._g7r).to(be_valid_block_generator)

            with after.all:
                self._api.delete_block_generator(self._g7r.id)
                self._api.delete_block_file(self._file.id)

            with context('POST'):
                with description('by existing ID'):
                    with it('is running'):
                        expect(self._g7r.running).to(be_true)

                    with it('stopped (204)'):
                        result = self._api.stop_block_generator_with_http_info(
                            self._g7r.id, _return_http_data_only=False)
                        expect(result[1]).to(equal(204))

                    with it('is not running'):
                        g7r = self._api.get_block_generator(self._g7r.id)
                        expect(g7r).to(be_valid_block_generator)
                        expect(g7r.running).to(be_false)

                    with description('already stopped generator'):
                        with it('bad request (400)'):
                            expr = lambda: self._api.stop_block_generator(self._g7r.id)
                            expect(expr).to(raise_api_exception(400))

                with description('by non-existent ID'):
                    with it('not found (404)'):
                        expr = lambda: self._api.stop_block_generator('unknown')
                        expect(expr).to(raise_api_exception(404))

                with description('by invalid ID'):
                    with it('bad request (400)'):
                        expr = lambda: self._api.stop_block_generator('bad_id')
                        expect(expr).to(raise_api_exception(400))

    with description('Block Generator bulk operations'):
        with description('/block-generators/x/bulk-create'):
            with context('PUT'):
                with it('not allowed (405)'):
                    expect(lambda: self._api.api_client.call_api('/block-generators/x/bulk-create', 'PUT')).to(
                        raise_api_exception(405, headers={'Allow': "POST"}))

            with context('POST'):
                with before.all:
                    self._file = self._api.create_block_file(
                        file_model(1024, '/tmp/foo'))
                    wait_for_file_initialization_done(self._api, self._file.id, 1)

                    self._models = [
                        block_generator_model(self._file.id)
                            for i in range(3)]
                    request = client.models.BulkCreateBlockGeneratorsRequest(self._models)
                    self._result = self._api.bulk_create_block_generators_with_http_info(
                        request, _return_http_data_only=False)

                with after.all:
                    for g7r in self._result[0]:
                        self._api.delete_block_generator(g7r.id)
                    self._api.delete_block_file(self._file.id)

                with it('created (200)'):
                    expect(self._result[1]).to(equal(200))

                with it('has Content-Type: application/json header'):
                    expect(self._result[2]).to(has_json_content_type)

                with it('returned valid generator list'):
                    expect(self._result[0]).not_to(be_empty)
                    expect(len(self._result[0])).to(equal(len(self._models)))
                    for g7r in self._result[0]:
                        expect(g7r).to(be_valid_block_generator)

                with it('has same config'):
                    for idx in range(len(self._models)):
                        model = self._models[idx]
                        if (not model.id):
                            model.id = self._result[0][idx].id
                        expect(self._result[0][idx]).to(equal(model))

        with description('/block-generators/x/bulk-delete'):
            with context('PUT'):
                with it('not allowed (405)'):
                    expect(lambda: self._api.api_client.call_api('/block-generators/x/bulk-delete', 'PUT')).to(
                        raise_api_exception(405, headers={'Allow': "POST"}))

            with context('POST'):
                with before.all:
                    self._ids = []
                    self._file = self._api.create_block_file(
                        file_model(1024, '/tmp/foo'))
                    expect(self._file).to(be_valid_block_file)
                    wait_for_file_initialization_done(self._api, self._file.id, 1)
                    self._model = block_generator_model(
                        self._file.id, running=False)

                with after.all:
                    self._api.delete_block_file(self._file.id)

                with shared_context('delete generators'):
                    with before.all:
                        self._g8s = [
                            self._api.create_block_generator(self._model)
                                for i in range(3)]

                    with it('all exists'):
                        for g7r in self._g8s:
                            result = self._api.get_block_generator(g7r.id)
                            expect(result).to(be_valid_block_generator)

                    with it('no content (204)'):
                        request = client.models.BulkDeleteBlockGeneratorsRequest(
                            [g7r.id for g7r in self._g8s] + self._ids)
                        result = self._api.bulk_delete_block_generators_with_http_info(
                            request, _return_http_data_only=False)
                        expect(result[1]).to(equal(204))

                    with it('all deleted'):
                        for g7r in self._g8s:
                            result = lambda: self._api.get_block_generator(g7r.id)
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

        with description('/block-generators/x/bulk-start'):
            with context('PUT'):
                with it('not allowed (405)'):
                    expect(lambda: self._api.api_client.call_api('/block-generators/x/bulk-start', 'PUT')).to(
                        raise_api_exception(405, headers={'Allow': "POST"}))

            with context('POST'):
                with before.all:
                    self._file = self._api.create_block_file(
                        file_model(1024, '/tmp/foo'))
                    expect(self._file).to(be_valid_block_file)
                    wait_for_file_initialization_done(self._api, self._file.id, 1)

                    self._g8s = [
                        self._api.create_block_generator(block_generator_model(self._file.id))
                            for i in range(3)]

                with after.all:
                    request = client.models.BulkDeleteBlockGeneratorsRequest(
                        [g7r.id for g7r in self._g8s])
                    self._api.bulk_delete_block_generators(request)
                    self._api.delete_block_file(self._file.id)

                with description('with existing IDs'):
                    with before.all:
                        request = client.models.BulkStartBlockGeneratorsRequest(
                            [g7r.id for g7r in self._g8s])
                        self._result = self._api.bulk_start_block_generators_with_http_info(
                            request, _return_http_data_only=False)

                    with it('is not running'):
                        for g7r in self._g8s:
                            expect(g7r.running).to(be_false)

                    with it('success (200)'):
                        expect(self._result[1]).to(equal(200))

                    with it('has Content-Type: application/json header'):
                        expect(self._result[2]).to(has_json_content_type)

                    with it('returned valid results'):
                        for result in self._result[0]:
                            expect(result).to(be_valid_block_generator_result)
                            expect(result.active).to(be_true)

                    with it('started'):
                        for g7r in self._g8s:
                            result = self._api.get_block_generator(g7r.id)
                            expect(result).to(be_valid_block_generator)
                            expect(result.running).to(be_true)

                    with description('already running generators'):
                        with it('bad request (400)'):
                            request = client.models.BulkStartBlockGeneratorsRequest(
                                [g7r.id for g7r in self._g8s])
                            expr = lambda: self._api.bulk_start_block_generators(request)
                            expect(expr).to(raise_api_exception(400))

                        with it('state was not changed'):
                            for g7r in self._g8s:
                                result = self._api.get_block_generator(g7r.id)
                                expect(result).to(be_valid_block_generator)
                                expect(result.running).to(be_true)

                with description('with non-existent ID'):
                    with before.all:
                        for num, g7r in enumerate(self._g8s, start=1):
                            try:
                                if (num % 2 == 0):
                                    g7r.running = False
                                    self._api.stop_block_generator(g7r.id)
                                else:
                                    g7r.running = True
                                    self._api.start_block_generator(g7r.id)
                            except Exception:
                                pass
                        self._results_count = len(self._api.list_block_generator_results())

                    with it('not found (404)'):
                        request = client.models.BulkStartBlockGeneratorsRequest(
                            [g7r.id for g7r in self._g8s] + ['unknown'])
                        expr = lambda: self._api.bulk_start_block_generators(request)
                        expect(expr).to(raise_api_exception(404))

                    with it('state was not changed'):
                        for g7r in self._g8s:
                            result = self._api.get_block_generator(g7r.id)
                            expect(result).to(be_valid_block_generator)
                            expect(result.running).to(equal(g7r.running))

                    with it('new results was not created'):
                        results = self._api.list_block_generator_results()
                        expect(len(results)).to(equal(self._results_count))

                with description('by invalid ID'):
                    with before.all:
                        self._results_count = len(self._api.list_block_generator_results())

                    with it('bad request (400)'):
                        request = client.models.BulkStartBlockGeneratorsRequest(
                            [g7r.id for g7r in self._g8s] + ['bad_id'])
                        expr = lambda: self._api.bulk_start_block_generators(request)
                        expect(expr).to(raise_api_exception(400))

                    with it('state was not changed'):
                        for g7r in self._g8s:
                            result = self._api.get_block_generator(g7r.id)
                            expect(result).to(be_valid_block_generator)
                            expect(result.running).to(equal(g7r.running))

                    with it('new results was not created'):
                        results = self._api.list_block_generator_results()
                        expect(len(results)).to(equal(self._results_count))

        with description('/block-generators/x/bulk-stop'):
            with context('PUT'):
                with it('not allowed (405)'):
                    expect(lambda: self._api.api_client.call_api('/block-generators/x/bulk-stop', 'PUT')).to(
                        raise_api_exception(405, headers={'Allow': "POST"}))

            with context('POST'):
                with before.all:
                    self._ids = []
                    self._file = self._api.create_block_file(
                        file_model(1024, '/tmp/foo'))
                    expect(self._file).to(be_valid_block_file)
                    wait_for_file_initialization_done(self._api, self._file.id, 1)

                    self._g8s = [
                        self._api.create_block_generator(
                            block_generator_model(self._file.id))
                                for i in range(3)]

                with after.all:
                    request = client.models.BulkDeleteBlockGeneratorsRequest(
                        [g7r.id for g7r in self._g8s])
                    self._api.bulk_delete_block_generators(request)
                    self._api.delete_block_file(self._file.id)

                with shared_context('stop generators'):
                    with before.all:
                        for g7r in self._g8s:
                            self._api.start_block_generator(g7r.id)

                    with it('all running'):
                        for g7r in self._g8s:
                            result = self._api.get_block_generator(g7r.id)
                            expect(result.running).to(be_true)

                    with it('no content (204)'):
                        request = client.models.BulkStopBlockGeneratorsRequest(
                            [g7r.id for g7r in self._g8s] + self._ids)
                        result = self._api.bulk_stop_block_generators_with_http_info(
                            request, _return_http_data_only=False)
                        expect(result[1]).to(equal(204))

                    with it('all stopped'):
                        for g7r in self._g8s:
                            result = self._api.get_block_generator(g7r.id)
                            expect(result).to(be_valid_block_generator)
                            expect(result.running).to(be_false)

                with description('with existing IDs'):
                    with included_context('stop generators'):
                        pass

                    with description('already stopped generators'):
                        with it('no content (204)'):
                            request = client.models.BulkStopBlockGeneratorsRequest(
                                [g7r.id for g7r in self._g8s])
                            result = self._api.bulk_stop_block_generators_with_http_info(
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

    with description('Block Generator Results'):
        with before.all:
            self._file = self._api.create_block_file(
                file_model(1024, '/tmp/foo'))
            expect(self._file).to(be_valid_block_file)
            wait_for_file_initialization_done(self._api, self._file.id, 1)

            self._g7r = self._api.create_block_generator(
                block_generator_model(self._file.id))
            expect(self._g7r).to(be_valid_block_generator)

            self._runs = 3;
            for i in range(self._runs):
                self._api.start_block_generator(self._g7r.id)
                self._api.stop_block_generator(self._g7r.id)

        with after.all:
            self._api.delete_block_file(self._file.id)

        with description('/block-generator-resuls'):
            with context('PUT'):
                with it('not allowed (405)'):
                    expect(lambda: self._api.api_client.call_api('/block-generator-results', 'PUT')).to(
                        raise_api_exception(405, headers={'Allow': "GET"}))

            with context('GET'):
                with before.all:
                    self._result = self._api.list_block_generator_results_with_http_info(
                        _return_http_data_only=False)

                with it('success (200)'):
                    expect(self._result[1]).to(equal(200))

                with it('has Content-Type: application/json header'):
                    expect(self._result[2]).to(has_json_content_type)

                with it('results list'):
                    expect(self._result[0]).not_to(be_empty)
                    expect(len(self._result[0])).to(be(self._runs))
                    for result in self._result[0]:
                        expect(result).to(be_valid_block_generator_result)

        with description('/block-generator-results/{id}'):
            with before.all:
                rlist = self._api.list_block_generator_results()
                expect(rlist).not_to(be_empty)
                self._result = rlist[0]

            with context('GET'):
                with description('by existing ID'):
                    with before.all:
                        self._get_result = self._api.get_block_generator_result_with_http_info(
                            self._result.id, _return_http_data_only=False)

                    with it('success (200)'):
                        expect(self._get_result[1]).to(equal(200))

                    with it('has Content-Type: application/json header'):
                        expect(self._get_result[2]).to(has_json_content_type)

                    with it('valid result'):
                        expect(self._get_result[0]).to(be_valid_block_generator_result)

                with description('by non-existent ID'):
                    with it('not found (404)'):
                        expr = lambda: self._api.get_block_generator_result('unknown')
                        expect(expr).to(raise_api_exception(404))

                with description('by invalid ID'):
                    with it('bad request (400)'):
                        expr = lambda: self._api.get_block_generator_result('bad_id')
                        expect(expr).to(raise_api_exception(400))

            with context('DELETE'):
                with description('by existing ID'):
                    with description('active result'):
                        with before.all:
                            self._result = self._api.start_block_generator(self._g7r.id)

                        with after.all:
                            self._api.stop_block_generator(self._g7r.id)

                        with it('exists'):
                            expect(self._result).to(be_valid_block_generator_result)

                        with it('active'):
                            expect(self._result.active).to(be_true)

                        with it('bad request (400)'):
                            result = lambda: self._api.delete_block_generator_result(self._result.id)
                            expect(result).to(raise_api_exception(400))

                        with it('not deleted'):
                            result = self._api.get_block_generator_result(self._result.id)
                            expect(result).to(be_valid_block_generator_result)

                    with description('inactive result'):
                        with before.all:
                            result = self._api.start_block_generator(self._g7r.id)
                            self._api.stop_block_generator(self._g7r.id)
                            self._result = self._api.get_block_generator_result(result.id)

                        with it('exists'):
                            expect(self._result).to(be_valid_block_generator_result)

                        with it('not active'):
                            expect(self._result.active).to(be_false)

                        with it('deleted (204)'):
                            result = self._api.delete_block_generator_result_with_http_info(
                                self._result.id, _return_http_data_only=False)
                            expect(result[1]).to(equal(204))

                        with it('not found (404)'):
                            expr = lambda: self._api.get_block_generator_result(self._result.id)
                            expect(expr).to(raise_api_exception(404))

                with description('by non-existent ID'):
                    with it('not found (404)'):
                        expr = lambda: self._api.delete_block_generator_result('unknown')
                        expect(expr).to(raise_api_exception(404))

                with description('by invalid ID'):
                    with it('bad request (400)'):
                        expr = lambda: self._api.delete_block_generator_result('bad_id')
                        expect(expr).to(raise_api_exception(400))

        with description('delete results with generator'):
            with it('results exists'):
                results = self._api.list_block_generator_results()
                expect(results).not_to(be_empty)

            with it('generator deleted'):
                result = self._api.delete_block_generator_with_http_info(
                    self._g7r.id, _return_http_data_only=False)
                expect(result[1]).to(equal(204))

            with it('results deleted'):
                results = self._api.list_block_generator_results()
                expect(results).to(be_empty)
