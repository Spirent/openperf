import os
import time
import client.api
import client.models
import psutil

from mamba import description, before, after, it
from expects import *
from expects.matchers import Matcher
from common import Config, Service
from common.helper import (make_dynamic_results_config,
                        check_modules_exists,
                        get_network_dynamic_results_fields,
                        network_generator_model,
                        network_server_model)
from common.matcher import (has_location,
                            has_json_content_type,
                            raise_api_exception,
                            be_valid_network_server,
                            be_valid_network_generator,
                            be_valid_network_generator_result,
                            be_valid_dynamic_results)


CONFIG = Config(os.path.join(os.path.dirname(__file__),
                os.environ.get('MAMBA_CONFIG', 'config.yaml')))


def clear_network_instances(api):
    try:
        for s in api.list_network_servers():
            api.delete_network_server(s.id)
    except AttributeError:
        pass
    try:
        for gen in api.list_network_generators():
            if gen.running:
                api.stop_network_generator(gen.id)
            api.delete_network_generator(gen.id)
    except AttributeError:
        pass
    try:
        for res in api.list_network_generator_results():
            api.delete_network_generator_result(res.id)
    except AttributeError:
        pass


def get_interface_by_address(addr):
    for iface, v in psutil.net_if_addrs().items():
        for item in v:
            if item.address == addr:
                return iface

def is_nonzero_op_result(op_result):
    return  (op_result.ops_target != 0 and
             op_result.ops_actual != 0 and
             op_result.bytes_target != 0 and
             op_result.bytes_actual != 0 and
             op_result.latency_total != 0)

def wait_for_nonzero_result(api, result_id, mode='rw', timeout=5.0):
    sleep_time = .1
    elapsed_time = 0
    while elapsed_time <= timeout:
        result = api.get_network_generator_result(result_id)
        expect(result).to(be_valid_network_generator_result)
        valid = ((not 'r' in mode or is_nonzero_op_result(result.read)) and
                 (not 'w' in mode or is_nonzero_op_result(result.write)))
        if valid:
            return result
        time.sleep(sleep_time)
        elapsed_time += sleep_time

    raise AssertionError('Failed waiting for nonzero %s result.  %s' % (mode, str(result)))


def wait_for_nonzero_server_result(api, server_id, timeout=5.0):
    sleep_time = 0.1
    stop_time = time.time() + timeout
    while time.time() <= stop_time:
        server = api.get_network_server(server_id)
        expect(server).to(be_valid_network_server)
        if server.stats.bytes_received > 0:
            return server
        time.sleep(sleep_time)

    raise AssertionError('Failed waiting for nonzero server result')


with description('Network Generator Module', 'network') as self:
    with shared_context('network_module'):
        with before.all:
            self._process = self._service.start()
            self._api = client.api.NetworkGeneratorApi(self._service.client())
            if not check_modules_exists(self._service.client(), 'network'):
                self.skip()

        with after.all:
            try:
                self._process.terminate()
                self._process.wait()
            except AttributeError:
                pass

        with description('Network Servers'):
            with description('/network/servers'):
                with context('PUT'):
                    with it('not allowed (405)'):
                        expect(lambda: self._api.api_client.call_api('/network/servers', 'PUT')).to(
                            raise_api_exception(405, headers={'Allow': "GET, POST"}))

                with context('POST'):
                    with shared_context('create server'):
                        with before.all:
                            self._result = self._api.create_network_server_with_http_info(
                                self._model, _return_http_data_only=False)

                        with after.all:
                            clear_network_instances(self._api)

                        with it('created'):
                            expect(self._result[1]).to(equal(201))

                        with it('has valid Location header'):
                            expect(self._result[2]).to(has_location('/network/servers/' + self._result[0].id))

                        with it('has Content-Type: application/json header'):
                            expect(self._result[2]).to(has_json_content_type)

                        with it('returned valid server'):
                            expect(self._result[0]).to(be_valid_network_server)

                        with it('has same config'):
                            if (not self._model.id):
                                self._model.id = self._result[0].id
                            self._model.stats = self._result[0].stats
                            expect(self._result[0]).to(equal(self._model))

                    with description('with empty ID'):
                        with before.all:
                            self._model = network_server_model(self._api.api_client, interface=self._ip4_server_interface)

                        with included_context('create server'):
                            with it('random ID assigned'):
                                expect(self._result[0].id).not_to(be_empty)

                    with description('with udp protocol'):
                        with before.all:
                            self._model = network_server_model(self._api.api_client, protocol='udp', interface=self._ip4_server_interface)

                        with included_context('create server'):
                            pass

                    with description('with specified ID'):
                        with before.all:
                            self._model = network_server_model(self._api.api_client, id='some-specified-id', interface=self._ip4_server_interface)

                        with included_context('create server'):
                            pass

                    with description('unknown protocol'):
                        with it('bad request (400)'):
                            model = network_server_model(
                                self._api.api_client, protocol='foo', interface=self._ip4_server_interface)
                            expr = lambda: self._api.create_network_server(model)
                            expect(expr).to(raise_api_exception(400))

                    with description('duplicate port'):
                        with after.all:
                            clear_network_instances(self._api)

                        with it('bad request (400)'):
                            model = network_server_model(
                                self._api.api_client, id = 'some-id-1', interface=self._ip4_server_interface)
                            self._api.create_network_server(model)
                            model = network_server_model(
                                self._api.api_client, id = 'some-id-2', interface=self._ip4_server_interface)
                            expr = lambda: self._api.create_network_server(model)
                            expect(expr).to(raise_api_exception(400))

                with context('GET'):
                    with before.all:
                        model = network_server_model(self._api.api_client, interface=self._ip4_server_interface)
                        self._servers = []
                        for a in range(3):
                            model.port = 3357+a
                            self._servers.append(self._api.create_network_server(model))
                        self._result = self._api.list_network_servers_with_http_info(
                            _return_http_data_only=False)

                    with after.all:
                        clear_network_instances(self._api)

                    with it('success (200)'):
                        expect(self._result[1]).to(equal(200))

                    with it('has Content-Type: application/json header'):
                        expect(self._result[2]).to(has_json_content_type)

                    with it('return list'):
                        expect(self._result[0]).not_to(be_empty)
                        for gen in self._result[0]:
                            expect(gen).to(be_valid_network_server)

            with description('/network/servers/{id}'):
                with before.all:
                    model = network_server_model(self._api.api_client, interface=self._ip4_server_interface)
                    server = self._api.create_network_server(model)
                    expect(server).to(be_valid_network_server)
                    self._server = server

                with context('GET'):
                    with description('by existing ID'):
                        with before.all:
                            self._result = self._api.get_network_server_with_http_info(
                                self._server.id, _return_http_data_only=False)

                        with it('success (200)'):
                            expect(self._result[1]).to(equal(200))

                        with it('has Content-Type: application/json header'):
                            expect(self._result[2]).to(has_json_content_type)

                        with it('server object'):
                            expect(self._result[0]).to(be_valid_network_server)

                    with description('by non-existent ID'):
                        with it('not found (404)'):
                            expr = lambda: self._api.get_network_server('unknown')
                            expect(expr).to(raise_api_exception(404))

                    with description('by invalid ID'):
                        with it('bad request (400)'):
                            expr = lambda: self._api.get_network_server('bad_id')
                            expect(expr).to(raise_api_exception(400))

                with context('DELETE'):
                    with description('by existing ID'):
                        with it('removed (204)'):
                            result = self._api.delete_network_server_with_http_info(
                                self._server.id, _return_http_data_only=False)
                            expect(result[1]).to(equal(204))

                        with it('not found (404)'):
                            expr = lambda: self._api.get_network_server(self._server.id)
                            expect(expr).to(raise_api_exception(404))

                    with description('by non-existent ID'):
                        with it('not found (404)'):
                            expr = lambda: self._api.delete_network_server('unknown')
                            expect(expr).to(raise_api_exception(404))

                    with description('by invalid ID'):
                        with it('bad request (400)'):
                            expr = lambda: self._api.delete_network_server('bad_id')
                            expect(expr).to(raise_api_exception(400))

        with description('Network Generators'):
            with description('/network/generators'):
                with context('PUT'):
                    with it('returns 405'):
                        expect(lambda: self._api.api_client.call_api('/network/generators', 'PUT')).to(
                            raise_api_exception(405, headers={'Allow': "GET, POST"}))

                with context('POST'):
                    with shared_context('create generator'):
                        with before.all:
                            self._result = self._api.create_network_generator_with_http_info(
                                self._model, _return_http_data_only=False)

                        with after.all:
                            clear_network_instances(self._api)

                        with it('created (201)'):
                            expect(self._result[1]).to(equal(201))

                        with it('has valid Location header'):
                            expect(self._result[2]).to(has_location('/network/generators/' + self._result[0].id))

                        with it('has Content-Type: application/json header'):
                            expect(self._result[2]).to(has_json_content_type)

                        with it('returned valid generator'):
                            expect(self._result[0]).to(be_valid_network_generator)

                        with it('has same config'):
                            if (not self._model.id):
                                self._model.id = self._result[0].id
                            expect(self._result[0]).to(equal(self._model))

                        with it('ratio empty'):
                            expect(self._result[0].config.ratio).to(be_none)

                    with description("with ratio"):
                        with before.all:
                            generator = network_generator_model(self._api.api_client, interface=self._ip4_client_interface)
                            generator.config.ratio = client.models.BlockGeneratorReadWriteRatio()
                            generator.config.ratio.reads = 1
                            generator.config.ratio.writes = 1
                            self._result = self._api.create_network_generator_with_http_info(generator)

                        with after.all:
                            clear_network_instances(self._api)

                        with it('created (201)'):
                            expect(self._result[1]).to(equal(201))

                        with it('has a valid network generator'):
                            expect(self._result[0]).to(be_valid_network_generator)

                        with it('ratio configured'):
                            expect(self._result[0].config.ratio).not_to(be_none)
                            expect(self._result[0].config.ratio.reads).to(be(1))
                            expect(self._result[0].config.ratio.writes).to(be(1))

                    with description('with empty ID'):
                        with before.all:
                            self._model = network_generator_model(self._api.api_client, interface=self._ip4_client_interface)

                        with included_context('create generator'):
                            with it('random ID assigned'):
                                expect(self._result[0].id).not_to(be_empty)

                    with description('with udp protocol'):
                        with before.all:
                            self._model = network_generator_model(self._api.api_client, protocol='udp', interface=self._ip4_client_interface)

                        with included_context('create generator'):
                            pass

                    with description('with specified ID'):
                        with before.all:
                            self._model = network_generator_model(self._api.api_client, id='some-specified-id', interface=self._ip4_client_interface)

                        with included_context('create generator'):
                            pass

                    with description('unknown protocol'):
                        with it('bad request (400)'):
                            model = network_generator_model(
                                self._api.api_client, protocol='foo', interface=self._ip4_client_interface)
                            expr = lambda: self._api.create_network_generator(model)
                            expect(expr).to(raise_api_exception(400))

                with context('GET'):
                    with before.all:
                        model = network_generator_model(self._api.api_client, interface=self._ip4_client_interface)
                        self._g8s = [self._api.create_network_generator(model) for a in range(3)]
                        self._result = self._api.list_network_generators_with_http_info(
                            _return_http_data_only=False)

                    with after.all:
                        clear_network_instances(self._api)

                    with it('success (200)'):
                        expect(self._result[1]).to(equal(200))

                    with it('has Content-Type: application/json header'):
                        expect(self._result[2]).to(has_json_content_type)

                    with it('return list'):
                        expect(self._result[0]).not_to(be_empty)
                        for gen in self._result[0]:
                            expect(gen).to(be_valid_network_generator)

            with description('/network/generators/{id}'):
                with before.all:
                    model = network_generator_model(self._api.api_client, interface=self._ip4_client_interface)
                    g7r = self._api.create_network_generator(model)
                    expect(g7r).to(be_valid_network_generator)
                    self._g7r = g7r

                with context('GET'):
                    with description('by existing ID'):
                        with before.all:
                            self._result = self._api.get_network_generator_with_http_info(
                                self._g7r.id, _return_http_data_only=False)

                        with it('success (200)'):
                            expect(self._result[1]).to(equal(200))

                        with it('has Content-Type: application/json header'):
                            expect(self._result[2]).to(has_json_content_type)

                        with it('generator object'):
                            expect(self._result[0]).to(be_valid_network_generator)

                    with description('by non-existent ID'):
                        with it('not found (404)'):
                            expr = lambda: self._api.get_network_generator('unknown')
                            expect(expr).to(raise_api_exception(404))

                    with description('by invalid ID'):
                        with it('bad request (400)'):
                            expr = lambda: self._api.get_network_generator('bad_id')
                            expect(expr).to(raise_api_exception(400))

                with context('DELETE'):
                    with description('by existing ID'):
                        with it('removed (204)'):
                            result = self._api.delete_network_generator_with_http_info(
                                self._g7r.id, _return_http_data_only=False)
                            expect(result[1]).to(equal(204))

                        with it('not found (404)'):
                            expr = lambda: self._api.get_network_generator(self._g7r.id)
                            expect(expr).to(raise_api_exception(404))

                    with description('by non-existent ID'):
                        with it('not found (404)'):
                            expr = lambda: self._api.delete_network_generator('unknown')
                            expect(expr).to(raise_api_exception(404))

                    with description('by invalid ID'):
                        with it('bad request (400)'):
                            expr = lambda: self._api.delete_network_generator('bad_id')
                            expect(expr).to(raise_api_exception(400))

            with description('/network/generators/{id}/start'):
                with before.all:
                    model = network_generator_model(self._api.api_client, interface=self._ip4_client_interface)
                    g7r = self._api.create_network_generator(model)
                    expect(g7r).to(be_valid_network_generator)
                    self._g7r = g7r

                with after.all:
                    clear_network_instances(self._api)

                with context('POST'):
                    with description('by existing ID'):
                        with before.all:
                            self._result = self._api.start_network_generator_with_http_info(
                                self._g7r.id, _return_http_data_only=False)

                        with it('is not running'):
                            expect(self._g7r.running).to(be_false)

                        with it('started (201)'):
                            expect(self._result[1]).to(equal(201))

                        with it('has valid Location header'):
                            expect(self._result[2]).to(has_location('/network/generator-results/' + self._result[0].id))

                        with it('has Content-Type: application/json header'):
                            expect(self._result[2]).to(has_json_content_type)

                        with it('returned valid result'):
                            expect(self._result[0]).to(be_valid_network_generator_result)
                            expect(self._result[0].active).to(be_true)
                            expect(self._result[0].generator_id).to(equal(self._g7r.id))

                        with it('is running'):
                            g7r = self._api.get_network_generator(self._g7r.id)
                            expect(g7r).to(be_valid_network_generator)
                            expect(g7r.running).to(be_true)

                    with description('by existing ID with Dynamic Results'):
                        with before.all:
                            self._api.stop_network_generator(self._g7r.id)
                            dynamic = make_dynamic_results_config(
                                get_network_dynamic_results_fields())
                            self._result = self._api.start_network_generator_with_http_info(
                                self._g7r.id, dynamic_results=dynamic, _return_http_data_only=False)

                        with it('is not running'):
                            expect(self._g7r.running).to(be_false)

                        with it('started (201)'):
                            expect(self._result[1]).to(equal(201))

                        with it('has valid Location header'):
                            expect(self._result[2]).to(has_location('/network/generator-results/' + self._result[0].id))

                        with it('has Content-Type: application/json header'):
                            expect(self._result[2]).to(has_json_content_type)

                        with it('returned valid result'):
                            expect(self._result[0]).to(be_valid_network_generator_result)
                            expect(self._result[0].active).to(be_true)
                            expect(self._result[0].generator_id).to(equal(self._g7r.id))
                            expect(self._result[0].dynamic_results).to(be_valid_dynamic_results)

                        with it('is running'):
                            g7r = self._api.get_network_generator(self._g7r.id)
                            expect(g7r).to(be_valid_network_generator)
                            expect(g7r.running).to(be_true)

                    with description('by non-existent ID'):
                        with it('not found (404)'):
                            expr = lambda: self._api.start_network_generator('unknown')
                            expect(expr).to(raise_api_exception(404))

                    with description('by invalid ID'):
                        with it('bad request (400)'):
                            expr = lambda: self._api.start_network_generator('bad_id')
                            expect(expr).to(raise_api_exception(400))

            with description('/network/generators/{id}/stop'):
                with before.all:
                    model = network_generator_model(self._api.api_client, interface=self._ip4_client_interface)
                    g7r = self._api.create_network_generator(model)
                    expect(g7r).to(be_valid_network_generator)

                    start_result = self._api.start_network_generator_with_http_info(g7r.id)
                    expect(start_result[1]).to(equal(201))

                    g7r = self._api.get_network_generator(g7r.id)
                    expect(g7r).to(be_valid_network_generator)
                    self._g7r = g7r

                with after.all:
                    clear_network_instances(self._api)

                with context('POST'):
                    with description('by existing ID'):
                        with it('is running'):
                            expect(self._g7r.running).to(be_true)

                        with it('stopped (204)'):
                            result = self._api.stop_network_generator_with_http_info(
                                self._g7r.id, _return_http_data_only=False)
                            expect(result[1]).to(equal(204))

                        with it('is not running'):
                            g7r = self._api.get_network_generator(self._g7r.id)
                            expect(g7r).to(be_valid_network_generator)
                            expect(g7r.running).to(be_false)

                    with description('by non-existent ID'):
                        with it('not found (404)'):
                            expr = lambda: self._api.start_network_generator('unknown')
                            expect(expr).to(raise_api_exception(404))

                    with description('by invalid ID'):
                        with it('bad request (400)'):
                            expr = lambda: self._api.start_network_generator('bad_id')
                            expect(expr).to(raise_api_exception(400))

            with description('/network/generators/x/bulk-start'):
                with before.all:
                    model = network_generator_model(self._api.api_client, interface=self._ip4_client_interface)
                    self._g8s = [self._api.create_network_generator(model) for a in range(3)]

                with after.all:
                    clear_network_instances(self._api)

                with context('PUT'):
                    with it('returns 405'):
                        expect(lambda: self._api.api_client.call_api('/network/generators/x/bulk-start', 'PUT')).to(
                            raise_api_exception(405, headers={'Allow': "POST"}))

                with description('POST'):
                    with description('by existing IDs'):
                        with before.all:
                            request = client.models.BulkStartNetworkGeneratorsRequest(
                                [g7r.id for g7r in self._g8s])
                            self._result = self._api.bulk_start_network_generators_with_http_info(
                                request, _return_http_data_only=False)

                        with it('is not running'):
                            for g7r in self._g8s:
                                expect(g7r.running).to(be_false)

                        with it('started (200)'):
                            expect(self._result[1]).to(equal(200))

                        with it('has Content-Type: application/json header'):
                            expect(self._result[2]).to(has_json_content_type)

                        with it('returned valid results'):
                            for result in self._result[0]:
                                expect(result).to(be_valid_network_generator_result)
                                expect(result.active).to(be_true)

                        with it('is running'):
                            for g7r in self._g8s:
                                result = self._api.get_network_generator(g7r.id)
                                expect(result).to(be_valid_network_generator)
                                expect(result.running).to(be_true)

                    with description('with non-existant ID'):
                        with it('is not running'):
                            request = client.models.BulkStopNetworkGeneratorsRequest(
                                [g7r.id for g7r in self._g8s])
                            self._api.bulk_stop_network_generators(request)
                            for g7r in self._g8s:
                                expect(g7r.running).to(be_false)

                        with it('not found (404)'):
                            request = client.models.BulkStartNetworkGeneratorsRequest(
                                [g7r.id for g7r in self._g8s] + ['unknown'])
                            expr = lambda: self._api.bulk_start_network_generators(request)
                            expect(expr).to(raise_api_exception(404))

                        with it('is not running'):
                            for g7r in self._g8s:
                                result = self._api.get_network_generator(g7r.id)
                                expect(result).to(be_valid_network_generator)
                                expect(result.running).to(be_false)

                    with description('with invalid ID'):
                        with it('is not running'):
                            request = client.models.BulkStopNetworkGeneratorsRequest(
                                [g7r.id for g7r in self._g8s])
                            self._api.bulk_stop_network_generators(request)
                            for g7r in self._g8s:
                                expect(g7r.running).to(be_false)

                        with it('bad request (400)'):
                            request = client.models.BulkStartNetworkGeneratorsRequest(
                                [g7r.id for g7r in self._g8s] + ['bad_id'])
                            expr = lambda: self._api.bulk_start_network_generators(request)
                            expect(expr).to(raise_api_exception(400))

                        with it('is not running'):
                            for g7r in self._g8s:
                                result = self._api.get_network_generator(g7r.id)
                                expect(result).to(be_valid_network_generator)
                                expect(result.running).to(be_false)

            with description('/network/generators/x/bulk-stop'):
                with before.all:
                    model = network_generator_model(
                        self._api.api_client, interface=self._ip4_client_interface)
                    g8s = [self._api.create_network_generator(model) for a in range(3)]

                    for a in range(3):
                        result = self._api.start_network_generator_with_http_info(g8s[a].id)
                        expect(result[1]).to(equal(201))

                    self._g8s = self._api.list_network_generators()
                    expect(len(self._g8s)).to(equal(3))

                with after.all:
                    clear_network_instances(self._api)

                with context('PUT'):
                    with it('not allowed (405)'):
                        expect(lambda: self._api.api_client.call_api('/network/generators/x/bulk-stop', 'PUT')).to(
                            raise_api_exception(405, headers={'Allow': "POST"}))

                with description('POST'):
                    with description('by existing IDs'):
                        with it('is running'):
                            for g7r in self._g8s:
                                expect(g7r.running).to(be_true)

                        with it('stopped (204)'):
                            request = client.models.BulkStopNetworkGeneratorsRequest(
                                [g7r.id for g7r in self._g8s])
                            expr = lambda: self._api.bulk_stop_network_generators(request)
                            expect(expr).not_to(raise_api_exception(204))

                        with it('is not running'):
                            for g7r in self._g8s:
                                result = self._api.get_network_generator(g7r.id)
                                expect(result).to(be_valid_network_generator)
                                expect(result.running).to(be_false)

                    with description('with non-existant ID'):
                        with it('is running'):
                            request = client.models.BulkStartNetworkGeneratorsRequest(
                                [g7r.id for g7r in self._g8s])
                            self._api.bulk_start_network_generators(request)
                            for g7r in self._g8s:
                                expect(g7r.running).to(be_true)

                        with it('not found (404)'):
                            request = client.models.BulkStopNetworkGeneratorsRequest(
                                [g7r.id for g7r in self._g8s] + ['unknown'])
                            expr = lambda: self._api.bulk_stop_network_generators(request)
                            expect(expr).to(raise_api_exception(404))

                        with it('is running'):
                            for g7r in self._g8s:
                                result = self._api.get_network_generator(g7r.id)
                                expect(result).to(be_valid_network_generator)
                                expect(result.running).to(be_true)

                    with description('with invalid ID'):
                        with it('is running'):
                            request = client.models.BulkStartNetworkGeneratorsRequest(
                                [g7r.id for g7r in self._g8s])
                            self._api.bulk_start_network_generators(request)
                            for g7r in self._g8s:
                                expect(g7r.running).to(be_true)

                        with it('bad request (400)'):
                            request = client.models.BulkStopNetworkGeneratorsRequest(
                                [g7r.id for g7r in self._g8s] + ['bad_id'])
                            expr = lambda: self._api.bulk_stop_network_generators(request)
                            expect(expr).to(raise_api_exception(400))

                        with it('is running'):
                            for g7r in self._g8s:
                                result = self._api.get_network_generator(g7r.id)
                                expect(result).to(be_valid_network_generator)
                                expect(result.running).to(be_true)

        with description('Network Generator Results'):
            with before.all:
                generator_model = network_generator_model(self._api.api_client, interface=self._ip4_client_interface)
                g7r = self._api.create_network_generator(generator_model)
                expect(g7r).to(be_valid_network_generator)

                result = self._api.start_network_generator(g7r.id)
                expect(result).to(be_valid_network_generator_result)
                self._api.stop_network_generator(g7r.id)

            with description('/network/generator-results'):
                with context('PUT'):
                    with it('not allowed (405)'):
                        expect(lambda: self._api.api_client.call_api('/network/generator-results', 'PUT')).to(
                            raise_api_exception(405, headers={'Allow': "GET"}))

                with context('GET'):
                    with before.all:
                        self._result = self._api.list_network_generator_results_with_http_info(
                            _return_http_data_only=False)

                    with it('success (200)'):
                        expect(self._result[1]).to(equal(200))

                    with it('has Content-Type: application/json header'):
                        expect(self._result[2]).to(has_json_content_type)

                    with it('results list'):
                        expect(self._result[0]).not_to(be_empty)
                        for result in self._result[0]:
                            expect(result).to(be_valid_network_generator_result)

            with description('/network/generator-results/{id}'):
                with before.all:
                    rlist = self._api.list_network_generator_results()
                    expect(rlist).not_to(be_empty)
                    self._result = rlist[0]

                with context('GET'):
                    with description('by existing ID'):
                        with before.all:
                            self._get_result = self._api.get_network_generator_result_with_http_info(
                                self._result.id, _return_http_data_only=False)

                        with it('success (200)'):
                            expect(self._get_result[1]).to(equal(200))

                        with it('has Content-Type: application/json header'):
                            expect(self._get_result[2]).to(has_json_content_type)

                        with it('valid result'):
                            expect(self._get_result[0]).to(be_valid_network_generator_result)

                    with description('by non-existent ID'):
                        with it('not found (404)'):
                            expr = lambda: self._api.get_network_generator_result('unknown')
                            expect(expr).to(raise_api_exception(404))

                    with description('by invalid ID'):
                        with it('bad request (400)'):
                            expr = lambda: self._api.get_network_generator_result('bad_id')
                            expect(expr).to(raise_api_exception(400))

                with context('DELETE'):
                    with description('by existing ID'):
                        with it('removed (204)'):
                            result = self._api.delete_network_generator_result_with_http_info(
                                self._result.id, _return_http_data_only=False)
                            expect(result[1]).to(equal(204))

                        with it('not found (404)'):
                            expr = lambda: self._api.get_network_generator_result(self._result.id)
                            expect(expr).to(raise_api_exception(404))

                    with description('by non-existent ID'):
                        with it('not found (404)'):
                            expr = lambda: self._api.delete_network_generator_result('unknown')
                            expect(expr).to(raise_api_exception(404))

                    with description('by invalid ID'):
                        with it('bad request (400)'):
                            expr = lambda: self._api.delete_network_generator_result('bad_id')
                            expect(expr).to(raise_api_exception(400))

            with description('Verify statistics'):
                with shared_context('check_statistics'):
                    with before.each:
                        clear_network_instances(self._api)

                        server_model = network_server_model(self._api.api_client, protocol=self._protocol, interface=self._server_interface)
                        self._server = self._api.create_network_server(server_model)
                        expect(self._server).to(be_valid_network_server)

                    with it('valid read result'):
                        generator_model = network_generator_model(self._api.api_client,
                                                                  protocol=self._protocol,
                                                                  interface=self._client_interface,
                                                                  host=self._host)
                        generator_model.config.writes_per_sec = 0
                        g7r = self._api.create_network_generator(generator_model)
                        expect(g7r).to(be_valid_network_generator)

                        self._result = self._api.start_network_generator(g7r.id)
                        expect(self._result).to(be_valid_network_generator_result)

                        wait_for_nonzero_result(self._api, self._result.id, 'r')

                        self._api.stop_network_generator(g7r.id)
                        result = self._api.get_network_generator_result(self._result.id)
                        server = wait_for_nonzero_server_result(self._api, self._server.id)
                        expect(server).to(be_valid_network_server)
                        expect(server.stats.connections).not_to(equal(0))
                        expect(server.stats.bytes_sent).not_to(equal(0))

                        #TODO compare server and client statistics after issue #429 fixed
                        #expect(server.stats.bytes_sent).to(equal(result.read.bytes_actual))

                    with it('valid write result'):
                        generator_model = network_generator_model(self._api.api_client,
                                                                  protocol=self._protocol,
                                                                  interface=self._client_interface,
                                                                  host=self._host)
                        generator_model.config.reads_per_sec = 0
                        g7r = self._api.create_network_generator(generator_model)
                        expect(g7r).to(be_valid_network_generator)

                        self._result = self._api.start_network_generator(g7r.id)
                        expect(self._result).to(be_valid_network_generator_result)

                        wait_for_nonzero_result(self._api, self._result.id, 'w')

                        self._api.stop_network_generator(g7r.id)
                        result = self._api.get_network_generator_result(self._result.id)
                        server = wait_for_nonzero_server_result(self._api, self._server.id)
                        expect(server).to(be_valid_network_server)
                        expect(server.stats.connections).not_to(equal(0))
                        expect(server.stats.bytes_received).not_to(equal(0))

                        #TODO compare server and client statistics after issue #429 fixed
                        #expect(server.stats.bytes_received).to(equal(result.write.bytes_actual))

                    with it('toggle generator'):
                        generator_model = network_generator_model(self._api.api_client,
                                                                    protocol=self._protocol,
                                                                    interface=self._client_interface,
                                                                    host=self._host)
                        g7r1 = self._api.create_network_generator(generator_model)
                        expect(g7r1).to(be_valid_network_generator)

                        generator_model = network_generator_model(self._api.api_client,
                                                                    protocol=self._protocol,
                                                                    interface=self._client_interface,
                                                                    host=self._host)
                        g7r2 = self._api.create_network_generator(generator_model)
                        expect(g7r2).to(be_valid_network_generator)

                        self._result1 = self._api.start_network_generator(g7r1.id)
                        expect(self._result1).to(be_valid_network_generator_result)

                        wait_for_nonzero_result(self._api, self._result1.id, 'rw')

                        # Switch to 2nd generator
                        request = client.models.ToggleNetworkGeneratorsRequest(g7r1.id, g7r2.id)
                        self._result2 = self._api.toggle_network_generators(request)
                        expect(self._result2).to(be_valid_network_generator_result)

                        # Get last result from read generator
                        result1 = self._api.get_network_generator_result(self._result1.id)
                        expect(result1).to(be_valid_network_generator_result)

                        # Verify generator state changed as expected
                        g7r = self._api.get_network_generator(g7r1.id)
                        expect(g7r.running).to(equal(False))
                        g7r = self._api.get_network_generator(g7r2.id)
                        expect(g7r.running).to(equal(True))

                        wait_for_nonzero_result(self._api, self._result2.id, 'rw')

                        self._api.stop_network_generator(g7r2.id)
                        result2 = self._api.get_network_generator_result(self._result2.id)
                        expect(result2).to(be_valid_network_generator_result)

                        server = wait_for_nonzero_server_result(self._api, self._server.id)
                        expect(server).to(be_valid_network_server)
                        expect(server.stats.connections).not_to(equal(0))
                        expect(server.stats.bytes_received).not_to(equal(0))

                    with it('toggle generator read to write'):
                        generator_model = network_generator_model(self._api.api_client,
                                                                    protocol=self._protocol,
                                                                    interface=self._client_interface,
                                                                    host=self._host)
                        generator_model.config.writes_per_sec = 0
                        g7r_read = self._api.create_network_generator(generator_model)
                        expect(g7r_read).to(be_valid_network_generator)

                        generator_model = network_generator_model(self._api.api_client,
                                                                    protocol=self._protocol,
                                                                    interface=self._client_interface,
                                                                    host=self._host)
                        generator_model.config.reads_per_sec = 0
                        g7r_write = self._api.create_network_generator(generator_model)
                        expect(g7r_write).to(be_valid_network_generator)

                        self._read_result = self._api.start_network_generator(g7r_read.id)
                        expect(self._read_result).to(be_valid_network_generator_result)

                        result = wait_for_nonzero_result(self._api, self._read_result.id, 'r')
                        expect(result.write.ops_target).to(equal(0))
                        expect(result.write.ops_actual).to(equal(0))
                        expect(result.write.bytes_target).to(equal(0))
                        expect(result.write.bytes_actual).to(equal(0))

                        # Switch to write generator
                        request = client.models.ToggleNetworkGeneratorsRequest(g7r_read.id, g7r_write.id)
                        self._write_result = self._api.toggle_network_generators(request)
                        expect(self._write_result).to(be_valid_network_generator_result)

                        # Get last result from read generator
                        read_result = self._api.get_network_generator_result(self._read_result.id)
                        expect(read_result).to(be_valid_network_generator_result)

                        # Verify generator state changed as expected
                        g7r = self._api.get_network_generator(g7r_read.id)
                        expect(g7r.running).to(equal(False))
                        g7r = self._api.get_network_generator(g7r_write.id)
                        expect(g7r.running).to(equal(True))

                        result = wait_for_nonzero_result(self._api, self._write_result.id, 'w')
                        expect(result.read.ops_target).to(equal(0))
                        expect(result.read.ops_actual).to(equal(0))
                        expect(result.read.bytes_target).to(equal(0))
                        expect(result.read.bytes_actual).to(equal(0))

                        self._api.stop_network_generator(g7r_write.id)
                        write_result = self._api.get_network_generator_result(self._write_result.id)
                        expect(write_result).to(be_valid_network_generator_result)

                        server = wait_for_nonzero_server_result(self._api, self._server.id)
                        expect(server).to(be_valid_network_server)
                        expect(server.stats.connections).not_to(equal(0))
                        expect(server.stats.bytes_received).not_to(equal(0))

                    with it('toggle generator write to read'):
                        generator_model = network_generator_model(self._api.api_client,
                                                                    protocol=self._protocol,
                                                                    interface=self._client_interface,
                                                                    host=self._host)
                        generator_model.config.writes_per_sec = 0
                        g7r_read = self._api.create_network_generator(generator_model)
                        expect(g7r_read).to(be_valid_network_generator)

                        generator_model = network_generator_model(self._api.api_client,
                                                                    protocol=self._protocol,
                                                                    interface=self._client_interface,
                                                                    host=self._host)
                        generator_model.config.reads_per_sec = 0
                        g7r_write = self._api.create_network_generator(generator_model)
                        expect(g7r_write).to(be_valid_network_generator)

                        self._write_result = self._api.start_network_generator(g7r_write.id)
                        expect(self._write_result).to(be_valid_network_generator_result)

                        result = wait_for_nonzero_result(self._api, self._write_result.id, 'w')
                        expect(result.read.ops_target).to(equal(0))
                        expect(result.read.ops_actual).to(equal(0))
                        expect(result.read.bytes_target).to(equal(0))
                        expect(result.read.bytes_actual).to(equal(0))

                        # Switch to write generator
                        request = client.models.ToggleNetworkGeneratorsRequest(g7r_write.id, g7r_read.id)
                        self._read_result = self._api.toggle_network_generators(request)
                        expect(self._read_result).to(be_valid_network_generator_result)

                        # Get last result from read generator
                        write_result = self._api.get_network_generator_result(self._write_result.id)
                        expect(write_result).to(be_valid_network_generator_result)

                        # Verify generator state changed as expected
                        g7r = self._api.get_network_generator(g7r_write.id)
                        expect(g7r.running).to(equal(False))
                        g7r = self._api.get_network_generator(g7r_read.id)
                        expect(g7r.running).to(equal(True))

                        result = wait_for_nonzero_result(self._api, self._read_result.id, 'r')
                        expect(result.write.ops_target).to(equal(0))
                        expect(result.write.ops_actual).to(equal(0))
                        expect(result.write.bytes_target).to(equal(0))
                        expect(result.write.bytes_actual).to(equal(0))

                        self._api.stop_network_generator(g7r_read.id)
                        read_result = self._api.get_network_generator_result(self._read_result.id)
                        expect(read_result).to(be_valid_network_generator_result)

                        server = wait_for_nonzero_server_result(self._api, self._server.id)
                        expect(server).to(be_valid_network_server)
                        expect(server.stats.connections).not_to(equal(0))
                        expect(server.stats.bytes_received).not_to(equal(0))

                with description('IPv4'):
                    with before.all:
                        self._host = self._ip4_host
                        self._server_interface = self._ip4_server_interface
                        self._client_interface = self._ip4_client_interface
                        if self._client_interface == None or self._server_interface == None:
                            self.skip()

                    with description('TCP'):
                        with before.all:
                            self._protocol = 'tcp'

                        with included_context('check_statistics'):
                            pass

                    with description('UDP'):
                        with before.all:
                            self._protocol = 'udp'

                        with included_context('check_statistics'):
                            pass

                with description('IPv6'):
                    with before.all:
                        self._host = self._ip6_host
                        self._server_interface = self._ip6_server_interface
                        self._client_interface = self._ip6_client_interface
                        if self._client_interface == None or self._server_interface == None:
                            self.skip()

                    with description('TCP'):
                        with before.all:
                            self._protocol = 'tcp'

                        with included_context('check_statistics'):
                            pass

                    with description('UDP'):
                        with before.all:
                            self._protocol = 'udp'

                        with included_context('check_statistics'):
                            pass

            with description('delete results with generator'):
                with it('results exists'):
                    results = self._api.list_network_generator_results()
                    expect(results).not_to(be_empty)

                with it('generator deleted'):
                    g8s = self._api.list_network_generators()
                    for g7r in g8s:
                        result = self._api.delete_network_generator_with_http_info(
                            g7r.id, _return_http_data_only=False)
                        expect(result[1]).to(equal(204))

                with it('results deleted'):
                    results = self._api.list_network_generator_results()
                    expect(results).to(be_empty)

            with after.all:
                clear_network_instances(self._api)

    with description('Driver KERNEL'):
        with before.all:
            self._service = Service(CONFIG.service())
            self._ip4_host = '127.0.0.1'
            iface = get_interface_by_address(self._ip4_host)
            self._ip4_server_interface = iface
            self._ip4_client_interface = iface
            self._ip6_host = '::1'
            iface = get_interface_by_address(self._ip6_host)
            self._ip6_server_interface = iface
            self._ip6_client_interface = iface

        with included_context('network_module'):
            pass

    with description('Driver DPDK'):
        with before.all:
            self._service = Service(CONFIG.service('dataplane'))
            self._ip4_host = '198.18.1.10'
            self._ip4_server_interface = 'server-v4'
            self._ip4_client_interface = 'client-v4'
            self._ip6_host = '2001:2::1'
            self._ip6_server_interface = 'server-v6'
            self._ip6_client_interface = 'client-v6'

        with included_context('network_module'):
            pass
