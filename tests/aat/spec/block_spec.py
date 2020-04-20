from mamba import description, before, after
from expects import *
import os

import client.api
import client.models
from common import Config, Service
from common.matcher import (be_valid_block_device,
                            be_valid_block_file,
                            be_valid_block_generator,
                            be_valid_block_generator_result,
                            raise_api_exception)


CONFIG = Config(os.path.join(os.path.dirname(__file__),
                             os.environ.get('MAMBA_CONFIG', 'config.yaml')))


def generator_model(resource_id = None):
    bg = client.models.BlockGenerator()
    bg.id = 'none'
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
    ba.id = 'none'
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


with description('Block,', 'block') as self:
    with description('REST API,'):
        with before.all:
            service = Service(CONFIG.service())
            self.process = service.start()
            self.api = client.api.BlockGeneratorApi(service.client())

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
            with description('empty id,'):
                with it('returns 400'):
                    file = file_model(1024, '/tmp/foo')
                    file.id = None
                    expect(lambda: self.api.create_block_file(file)).to(raise_api_exception(400))

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

        with description('create file,'):
            with description('empty id,'):
                with it('returns 400'):
                    file = file_model(1024, '/tmp/foo')
                    file.id = None
                    expect(lambda: self.api.create_block_file(file)).to(raise_api_exception(400))

            with description('invalid path'):
                with it('returns 400'):
                    file = file_model(1024, '/tmp/foo/foo')
                    expect(lambda: self.api.create_block_file(file)).to(raise_api_exception(400))

        with description('list generators,'):
            with before.each:
                file = self.api.create_block_file(file_model(1024, '/tmp/foo'))
                expect(file).to(be_valid_block_file)
                self.file = file
                gen = self.api.create_block_generator(generator_model(file.id))
                expect(gen).to(be_valid_block_generator)
                self.gen = gen

            with it('succeeds'):
                generators = self.api.list_generators()
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
            with description('by existing c id,'):
                with before.each:
                    file = self.api.create_block_file(file_model(1024, '/tmp/foo'))
                    expect(file).to(be_valid_block_file)
                    self.file = file
                    gen = self.api.create_block_generator(generator_model(file.id))
                    expect(gen).to(be_valid_block_generator)
                    self.gen = gen

                with it('succeeds'):
                    self.api.delete_block_generator(self.gen.id)
                    generators = self.api.list_generators()
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
                gen = self.api.create_block_generator(generator_model(file.id))
                expect(gen).to(be_valid_block_generator)
                self.gen = gen

            with description('by existing id,'):
                with it('succeeds'):
                    result = self.api.start_block_generator(self.gen.id)
                    expect(result).to(be_valid_block_generator_result)
                    expect(self.api.get_block_generator(self.gen.id).running).to(be_true)

            with description('running generator,'):
                with it('returns 400'):
                    self.api.start_block_generator(self.gen.id)
                    expect(lambda: self.api.start_block_generator(self.gen.id)).to(raise_api_exception(400))

            with description('non-existent id,'):
                with it('returns 404'):
                    expect(lambda: self.api.start_block_generator('foo')).to(raise_api_exception(404))

            with description('invalid id,'):
                with it('returns 404'):
                    expect(lambda: self.api.start_block_generator('f_oo')).to(raise_api_exception(404))

            with after.each:
                for gen in self.api.list_generators():
                    if gen.running:
                        self.api.stop_block_generator(gen.id)

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
                for gen in self.api.list_generators():
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
                for gen in self.api.list_generators():
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
                for gen in self.api.list_generators():
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
                for gen in self.api.list_generators():
                    if gen.running:
                        self.api.stop_block_generator(gen.id)

        with after.each:
            try:
                for gen in self.api.list_generators():
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
