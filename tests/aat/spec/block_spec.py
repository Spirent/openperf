import os
import client.api
import client.models

from mamba import description, before, after
from expects import *
from expects.matchers import Matcher
from common import Config, Service
from common.helper import make_dynamic_results_config
from common.helper import check_modules_exists
from common.matcher import (be_valid_block_device,
                            be_valid_block_file,
                            be_valid_block_generator,
                            be_valid_block_generator_result,
                            raise_api_exception,
                            be_valid_dynamic_results)


CONFIG = Config(os.path.join(os.path.dirname(__file__),
                             os.environ.get('MAMBA_CONFIG', 'config.yaml')))


def get_dynamic_results_fields():
    fields = []
    swagger_types = client.models.BlockGeneratorStats.swagger_types
    for (name, type) in swagger_types.items():
        if type in ['int', 'float']:
            fields.append('read.' + name)
            fields.append('write.' + name)
    return fields


def generator_model(resource_id = None):
    bg = client.models.BlockGenerator()
    bg.id = ''
    bg.resource_id = resource_id
    bg.running = False
    bg.config = client.models.BlockGeneratorConfig()
    bg.config.queue_depth = 1
    bg.config.reads_per_sec = 1
    bg.config.read_size = 1
    bg.config.writes_per_sec = 1
    bg.config.write_size = 1
    bg.config.pattern = "random"
    return bg


def file_model(file_size = None, path = None):
    ba = client.models.BlockFile()
    ba.id = ''
    ba.file_size = file_size
    ba.path = path
    ba.init_percent_complete = 0
    ba.state = 'none'
    return ba


def bulk_start_model(ids):
    bsbgr = client.models.BulkStartBlockGeneratorsRequest()
    bsbgr.ids = ids
    return bsbgr


def bulk_stop_model(ids):
    bsbgr = client.models.BulkStopBlockGeneratorsRequest()
    bsbgr.ids = ids
    return bsbgr

class has_location(Matcher):
    def __init__(self, expected):
        self._expected = CONFIG.service().base_url + expected

    def _match(self, subject):
        expect(subject).to(have_key('Location'))
        return subject['Location'] == self._expected, []

with description('Block,', 'block') as self:
    with description('REST API,'):
        with before.all:
            service = Service(CONFIG.service())
            self.process = service.start()
            self.api = client.api.BlockGeneratorApi(service.client())
            if not check_modules_exists(service.client(), 'block'):
                self.skip()

        with description('invalid HTTP methods,'):
            with description('/block-devices,'):
                with it('returns 405'):
                    expect(lambda: self.api.api_client.call_api('/block-devices', 'PUT')).to(
                        raise_api_exception(405, headers={'Allow': "GET"}))

            with description('/block-files,'):
                with it('returns 405'):
                    expect(lambda: self.api.api_client.call_api('/block-files', 'PUT')).to(
                        raise_api_exception(405, headers={'Allow': "GET, POST"}))

            with description('/block-generators,'):
                with it('returns 405'):
                    expect(lambda: self.api.api_client.call_api('/block-generators', 'PUT')).to(
                        raise_api_exception(405, headers={'Allow': "GET, POST"}))

            with description('/block-generator-results,'):
                with it('returns 405'):
                    expect(lambda: self.api.api_client.call_api('/block-generator-results', 'PUT')).to(
                        raise_api_exception(405, headers={'Allow': "GET"}))

        with description('list devices,'):
            with it('succeeds'):
                devices = self.api.list_block_devices()
                expect(devices).not_to(be_empty)
                for dev in devices:
                    expect(dev).to(be_valid_block_device)

        with description('get device,'):
            with description('by existing id,'):
                with it('returns a device'):
                    devices = self.api.list_block_devices()
                    expect(devices).not_to(be_empty)
                    device = self.api.get_block_device(devices[0].id)
                    expect(device).to(be_valid_block_device)

            with description('non-existent device,'):
                with it('returns 404'):
                    expect(lambda: self.api.get_block_device('foo')).to(raise_api_exception(404))

            with description('invalid id,'):
                with it('returns 404'):
                    expect(lambda: self.api.get_block_device('f_oo')).to(raise_api_exception(404))

        with description('list files,'):
            with before.each:
                file = self.api.create_block_file(file_model(1024, '/tmp/foo'))
                expect(file).to(be_valid_block_file)
                self.file = file

            with it('succeeds'):
                files = self.api.list_block_files()
                expect(files).not_to(be_empty)
                for dev in files:
                    expect(dev).to(be_valid_block_file)

        with description('get file,'):
            with before.each:
                file = self.api.create_block_file(file_model(1024, '/tmp/foo'))
                expect(file).to(be_valid_block_file)
                self.file = file

            with description('by existing id,'):
                with it('returns a file'):
                    file = self.api.get_block_file(self.file.id)
                    expect(file).to(be_valid_block_file)

            with description('non-existent file,'):
                with it('returns 404'):
                    expect(lambda: self.api.get_block_file('foo')).to(raise_api_exception(404))

            with description('invalid id,'):
                with it('returns 404'):
                    expect(lambda: self.api.get_block_file('f_oo')).to(raise_api_exception(404))

        with description('create file,'):
            with description("succeeds,"):
                with before.all:
                    self._result = self.api.create_block_file_with_http_info(file_model(1024, '/tmp/foo'))

                with it('succeeded'):
                    expect(self._result[1]).to(equal(201))

                with it('has valid Location header,'):
                    expect(self._result[2]).to(has_location('/block-files/' + self._result[0].id))

                with it('has a valid block file'):
                    expect(self._result[0]).to(be_valid_block_file)

            with description('empty id,'):
                with it('succeeded'):
                    file = file_model(1024, '/tmp/foo')
                    file.id = None
                    self._result = self.api.create_block_file_with_http_info(file)
                    expect(self._result[1]).to(equal(201))

            with description('invalid path'):
                with it('returns 400'):
                    file = file_model(1024, '/tmp/foo/foo')
                    expect(lambda: self.api.create_block_file(file)).to(raise_api_exception(400))

        with description('delete file,'):
            with before.each:
                file = self.api.create_block_file(file_model(1024, '/tmp/foo'))
                expect(file).to(be_valid_block_file)
                self.file = file

            with description('by existing id,'):
                with it('returns a file'):
                    self.api.delete_block_file(self.file.id)
                    expect(self.api.list_block_files()).to(be_empty)

            with description('non-existent file,'):
                with it('returns 404'):
                    expect(lambda: self.api.delete_block_file('foo')).to(raise_api_exception(404))

            with description('invalid id,'):
                with it('returns 404'):
                    expect(lambda: self.api.delete_block_file('f_oo')).to(raise_api_exception(404))

        with description('bulk create file,'):
            with description("succeeds,"):
                with before.all:
                    self._result = self.api.bulk_create_block_files_with_http_info(
                        client.models.BulkCreateBlockFilesRequest([
                            file_model(1024, '/tmp/foo1'),
                            file_model(1024, '/tmp/foo2'),
                        ])
                    )

                with it('succeeded'):
                    expect(self._result[1]).to(equal(200))

                with it('has a valid block files'):
                    expect(self._result[0]).not_to(be_empty)
                    expect(len(self._result[0])).to(equal(2))
                    for file in self._result[0]:
                        expect(file).to(be_valid_block_file)

            with description('invalid id,'):
                with it('returns 400'):
                    file = file_model(1024, '/tmp/foo')
                    file.id = "file_1"
                    expect(lambda: self.api.bulk_create_block_files(
                        client.models.BulkCreateBlockFilesRequest([
                            file_model(1024, '/tmp/foo1'),
                            file,
                        ])
                    )).to(raise_api_exception(400))

                with it('all or nothing'):
                    expect(self.api.list_block_files()).to(be_empty)

            with description('invalid path'):
                with it('returns 400'):
                    expect(lambda: self.api.bulk_create_block_files(
                        client.models.BulkCreateBlockFilesRequest([
                            file_model(1024, '/tmp/foo1'),
                            file_model(1024, '/tmp/foo/foo'),
                        ])
                    )).to(raise_api_exception(400))

                with it('all or nothing'):
                    expect(self.api.list_block_files()).to(be_empty)

        with description('bulk delete file,'):
            with before.each:
                file = self.api.create_block_file(file_model(1024, '/tmp/foo'))
                expect(file).to(be_valid_block_file)
                self.file = file

            with description('by existing id,'):
                with it('deletes a file'):
                    self.api.bulk_delete_block_files(client.models.BulkDeleteBlockFilesRequest([self.file.id]))
                    expect(self.api.list_block_files()).to(be_empty)

            with description('non-existent file,'):
                with it('returns 400'):
                    expect(lambda: self.api.bulk_delete_block_files(client.models.BulkDeleteBlockFilesRequest(['foo']))).to(raise_api_exception(400))

            with description('invalid id,'):
                with it('returns 400'):
                    expect(lambda: self.api.bulk_delete_block_files(client.models.BulkDeleteBlockFilesRequest(['f_oo']))).to(raise_api_exception(400))

        with description('list generators,'):
            with before.each:
                file = self.api.create_block_file(file_model(1024, '/tmp/foo'))
                expect(file).to(be_valid_block_file)
                self.file = file
                gen = self.api.create_block_generator(generator_model(file.id))
                expect(gen).to(be_valid_block_generator)
                self.gen = gen

            with it('succeeds'):
                generators = self.api.list_block_generators()
                expect(generators).not_to(be_empty)
                for gen in generators:
                    expect(gen).to(be_valid_block_generator)

        with description('get generator,'):
            with description('by existing id,'):
                with before.each:
                    file = self.api.create_block_file(file_model(1024, '/tmp/foo'))
                    expect(file).to(be_valid_block_file)
                    self.file = file
                    gen = self.api.create_block_generator(generator_model(file.id))
                    expect(gen).to(be_valid_block_generator)
                    self.gen = gen

                with it('succeeds'):
                    expect(self.api.get_block_generator(self.gen.id)).to(be_valid_block_generator)

            with description('non-existent generator,'):
                with it('returns 404'):
                    expect(lambda: self.api.get_block_generator('foo')).to(raise_api_exception(404))

            with description('invalid id,'):
                with it('returns 404'):
                    expect(lambda: self.api.get_block_generator('f_oo')).to(raise_api_exception(404))

        with description('create generator,'):
            with description("succeeds,"):
                with before.all:
                    file = self.api.create_block_file(file_model(1024, '/tmp/foo'))
                    expect(file).to(be_valid_block_file)
                    self.file = file
                    self._result = self.api.create_block_generator_with_http_info(generator_model(file.id))

                with it('succeeded'):
                    expect(self._result[1]).to(equal(201))

                with it('has valid Location header,'):
                    expect(self._result[2]).to(has_location('/block-generators/' + self._result[0].id))

                with it('has a valid block generator'):
                    expect(self._result[0]).to(be_valid_block_generator)

                with it('ratio empty'):
                    expect(self._result[0].config.ratio).to(be_none)

            with description("with ratio,"):
                with before.all:
                    file = self.api.create_block_file(file_model(1024, '/tmp/foo'))
                    expect(file).to(be_valid_block_file)
                    generator = generator_model(file.id)
                    generator.config.ratio = client.models.BlockGeneratorReadWriteRatio()
                    generator.config.ratio.reads = 1
                    generator.config.ratio.writes = 1
                    self._result = self.api.create_block_generator_with_http_info(generator)

                with it('succeeded'):
                    expect(self._result[1]).to(equal(201))

                with it('has a valid block generator'):
                    expect(self._result[0]).to(be_valid_block_generator)

                with it('ratio configured'):
                    expect(self._result[0].config.ratio).not_to(be_none)
                    expect(self._result[0].config.ratio.reads).to(be(1))
                    expect(self._result[0].config.ratio.writes).to(be(1))

            with description('empty source id,'):
                with it('returns 400'):
                    gen = generator_model()
                    gen.id = None
                    expect(lambda: self.api.create_block_generator(gen)).to(raise_api_exception(400))

            with description('non-existent source id,'):
                with it('returns 400'):
                    gen = generator_model()
                    gen.id = 'f_oo'
                    expect(lambda: self.api.create_block_generator(gen)).to(raise_api_exception(400))

            with description('invalid pattern'):
                with it('returns 400'):
                    gen = generator_model()
                    gen.config.pattern = 'foo'
                    expect(lambda: self.api.create_block_generator(gen)).to(raise_api_exception(400))

            with description('invalid resource_id'):
                with it('returns 400'):
                    gen = generator_model()
                    gen.resource_id = 'f_oo'
                    expect(lambda: self.api.create_block_generator(gen)).to(raise_api_exception(400))

            with description('non-existent resource_id'):
                with it('returns 400'):
                    gen = generator_model()
                    gen.resource_id = 'foo'
                    expect(lambda: self.api.create_block_generator(gen)).to(raise_api_exception(400))

        with description('delete generator,'):
            with description('by existing id,'):
                with before.each:
                    file = self.api.create_block_file(file_model(1024, '/tmp/foo'))
                    expect(file).to(be_valid_block_file)
                    self.file = file
                    gen = self.api.create_block_generator(generator_model(file.id))
                    expect(gen).to(be_valid_block_generator)
                    self.gen = gen

                with it('succeeds'):
                    self.api.delete_block_generator(self.gen.id)
                    generators = self.api.list_block_generators()
                    expect(generators).to(be_empty)

                with description('non-existent id,'):
                    with it('returns 404'):
                        expect(lambda: self.api.delete_block_generator("foo")).to(raise_api_exception(404))

                with description('invalid id,'):
                    with it('returns 404'):
                        expect(lambda: self.api.delete_block_generator("invalid_id")).to(raise_api_exception(404))

        with description('start generator,'):
            with before.each:
                file = self.api.create_block_file(file_model(1024, '/tmp/foo'))
                expect(file).to(be_valid_block_file)
                self.file = file
                gen = self.api.create_block_generator(generator_model(self.file.id))
                expect(gen).to(be_valid_block_generator)
                self.gen = gen

            with description('by existing id,'):
                with it('succeeded'):
                    result = self.api.start_block_generator_with_http_info(self.gen.id)
                    expect(result[1]).to(equal(201))
                    expect(result[2]).to(has_location('/block-generator-results/' + result[0].id))
                    expect(result[0]).to(be_valid_block_generator_result)

            with description('running generator,'):
                with it('returns 400'):
                    self.api.start_block_generator(self.gen.id)
                    expect(lambda: self.api.start_block_generator(self.gen.id)).to(raise_api_exception(400))

            with description('by existing ID with Dynamic Results'):
                with it('started'):
                    dynamic = make_dynamic_results_config(get_dynamic_results_fields())
                    result = self.api.start_block_generator_with_http_info(
                        self.gen.id, dynamic_results=dynamic, _return_http_data_only=False)
                    expect(result[1]).to(equal(201))
                    expect(result[2]).to(has_location('/block-generator-results/' + result[0].id))
                    expect(result[0]).to(be_valid_block_generator_result)
                    expect(result[0].active).to(be_true)
                    expect(result[0].generator_id).to(equal(self.gen.id))
                    expect(result[0].dynamic_results).to(be_valid_dynamic_results)

            with description('non-existent id,'):
                with it('returns 404'):
                    expect(lambda: self.api.start_block_generator('foo')).to(raise_api_exception(404))

            with description('invalid id,'):
                with it('returns 404'):
                    expect(lambda: self.api.start_block_generator('f_oo')).to(raise_api_exception(404))

        with description('stop running generator,'):
            with before.each:
                file = self.api.create_block_file(file_model(1024, '/tmp/foo'))
                expect(file).to(be_valid_block_file)
                self.file = file
                gen = self.api.create_block_generator(generator_model(file.id))
                expect(gen).to(be_valid_block_generator)
                self.gen = gen
                self.api.start_block_generator(self.gen.id)

            with description('by existing id,'):
                with it('succeeds'):
                    self.api.stop_block_generator(self.gen.id)
                    expect(self.api.get_block_generator(self.gen.id).running).to(be_false)

            with description('non-existent id,'):
                with it('returns 404'):
                    expect(lambda: self.api.stop_block_generator('foo')).to(raise_api_exception(404))

            with description('invalid id,'):
                with it('returns 404'):
                    expect(lambda: self.api.stop_block_generator('f_oo')).to(raise_api_exception(404))

        with description('bulk start generators,'):
            with before.each:
                file = self.api.create_block_file(file_model(1024, '/tmp/foo'))
                expect(file).to(be_valid_block_file)
                self.file = file
                gen = self.api.create_block_generator(generator_model(file.id))
                expect(gen).to(be_valid_block_generator)
                self.gen = gen

            with description('by existing id,'):
                with it('succeeds'):
                    results = self.api.bulk_start_block_generators(bulk_start_model([self.gen.id]))
                    expect(results).not_to(be_empty)
                    for res in results:
                        expect(res).to(be_valid_block_generator_result)

            with description('running generator,'):
                with it('returns 400'):
                    self.api.start_block_generator(self.gen.id)
                    expect(lambda: self.api.bulk_start_block_generators(bulk_start_model([self.gen.id]))).to(raise_api_exception(400))

            with description('non-existent id,'):
                with it('returns 400'):
                    expect(lambda: self.api.bulk_start_block_generators(bulk_start_model(['foo']))).to(raise_api_exception(400))

            with description('invalid id,'):
                with it('returns 400'):
                    expect(lambda: self.api.bulk_start_block_generators(bulk_start_model(['f_oo']))).to(raise_api_exception(400))

            with after.each:
                for gen in self.api.list_block_generators():
                    if gen.running:
                        self.api.stop_block_generator(gen.id)

        with description('bulk stop generators,'):
            with before.each:
                file = self.api.create_block_file(file_model(1024, '/tmp/foo'))
                expect(file).to(be_valid_block_file)
                self.file = file
                gen = self.api.create_block_generator(generator_model(file.id))
                expect(gen).to(be_valid_block_generator)
                self.gen = gen
                res = self.api.start_block_generator(gen.id)
                expect(res).to(be_valid_block_generator_result)
                self.res = res

            with description('by existing id,'):
                with it('succeeds'):
                    self.api.bulk_stop_block_generators(bulk_stop_model([self.gen.id]))
                    res = self.api.get_block_generator_result(self.res.id)
                    expect(res).to(be_valid_block_generator_result)
                    expect(res.active).to(be_false)

            with description('non-existent id,'):
                with it('returns 400'):
                    expect(lambda: self.api.bulk_stop_block_generators(bulk_stop_model(['foo']))).to(raise_api_exception(400))

            with description('invalid id,'):
                with it('returns 400'):
                    expect(lambda: self.api.bulk_stop_block_generators(bulk_stop_model(['f_oo']))).to(raise_api_exception(400))

            with after.each:
                for gen in self.api.list_block_generators():
                    if gen.running:
                        self.api.stop_block_generator(gen.id)

        with description('list generator results,'):
            with before.each:
                file = self.api.create_block_file(file_model(1024, '/tmp/foo'))
                expect(file).to(be_valid_block_file)
                self.file = file
                gen = self.api.create_block_generator(generator_model(file.id))
                expect(gen).to(be_valid_block_generator)
                self.gen = gen
                res = self.api.start_block_generator(self.gen.id)
                expect(res).to(be_valid_block_generator_result)
                self.res = res

            with description('unfiltered,'):
                with it('succeeds'):
                    results = self.api.list_block_generator_results()
                    expect(results).not_to(be_empty)
                    for result in results:
                        expect(result).to(be_valid_block_generator_result)

            with description('by id,'):
                with it('succeeds'):
                    res = self.api.get_block_generator_result(self.res.id)
                    expect(res).to(be_valid_block_generator_result)

            with description('non active,'):
                with it('succeeds'):
                    res = self.api.get_block_generator_result(self.res.id)
                    expect(res).to(be_valid_block_generator_result)
                    expect(res.active).to(be_true)
                    self.api.stop_block_generator(self.gen.id)
                    res = self.api.get_block_generator_result(self.res.id)
                    expect(res).to(be_valid_block_generator_result)
                    expect(res.active).to(be_false)

            with description('non-existent id,'):
                with it('returns 404'):
                    expect(lambda: self.api.get_block_generator_result('foo')).to(raise_api_exception(404))

            with description('invalid id,'):
                with it('returns 404'):
                    expect(lambda: self.api.get_block_generator_result('f_oo')).to(raise_api_exception(404))

            with after.each:
                for gen in self.api.list_block_generators():
                    if gen.running:
                        self.api.stop_block_generator(gen.id)

        with description('delete generator results,'):
            with before.each:
                file = self.api.create_block_file(file_model(1024, '/tmp/foo'))
                expect(file).to(be_valid_block_file)
                self.file = file
                gen = self.api.create_block_generator(generator_model(file.id))
                expect(gen).to(be_valid_block_generator)
                self.gen = gen
                res = self.api.start_block_generator(self.gen.id)
                expect(res).to(be_valid_block_generator_result)
                self.res = res

            with description('by id,'):
                with it('succeeds'):
                    self.api.stop_block_generator(self.gen.id)
                    self.api.delete_block_generator_result(self.res.id)

            with description('non-existent id,'):
                with it('returns 404'):
                    expect(lambda: self.api.delete_block_generator_result('foo')).to(raise_api_exception(404))

            with description('invalid id,'):
                with it('returns 404'):
                    expect(lambda: self.api.delete_block_generator_result('f_oo')).to(raise_api_exception(404))

            with after.each:
                for gen in self.api.list_block_generators():
                    if gen.running:
                        self.api.stop_block_generator(gen.id)

        with after.each:
            try:
                for gen in self.api.list_block_generators():
                    if gen.running:
                        self.api.stop_block_generator(gen.id)
                    self.api.delete_block_generator(gen.id)
            except AttributeError:
                pass

            try:
                for file in self.api.list_block_files():
                    self.api.delete_block_file(file.id)
            except AttributeError:
                pass

            try:
                for file in self.api.list_block_generator_results():
                    self.api.delete_block_generator_results(file.id)
            except AttributeError:
                pass

            self.gen = None
            self.file = None

        with after.all:
            try:
                self.process.terminate()
                self.process.wait()
            except AttributeError:
                pass
