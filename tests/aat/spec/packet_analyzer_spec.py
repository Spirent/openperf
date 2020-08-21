from mamba import description, before, after
from expects import *
import os

import client.api
import client.models
from common import Config, Service
from common.matcher import (be_valid_packet_analyzer,
                            be_valid_packet_analyzer_result,
                            raise_api_exception)
from common.helper import check_modules_exists


CONFIG = Config(os.path.join(os.path.dirname(__file__),
                             os.environ.get('MAMBA_CONFIG', 'config.yaml')))


def get_first_port_id(api_client):
    ports_api = client.api.PortsApi(api_client)
    ports = ports_api.list_ports()
    expect(ports).not_to(be_empty)
    return ports[0].id


def get_second_port_id(api_client):
    ports_api = client.api.PortsApi(api_client)
    ports = ports_api.list_ports()
    expect(len(ports)).to(be_above(1))
    return ports[1].id


def analyzer_model(api_client, protocol_counters = None, flow_counters = None):
    config = client.models.PacketAnalyzerConfig()
    if protocol_counters:
        config.protocol_counters = protocol_counters
    if flow_counters:
        config.flow_counters = flow_counters

    ta = client.models.PacketAnalyzer()
    ta.source_id = get_first_port_id(api_client)
    ta.config = config

    return ta


def analyzer_models(api_client, protocol_counters = None, flow_counters = None):
    ana1 = analyzer_model(api_client, protocol_counters, flow_counters)
    ana2 = analyzer_model(api_client, protocol_counters, flow_counters)
    ana2.source_id = get_second_port_id(api_client)
    return [ana1, ana2]


with description('Packet Analyzer,', 'packet_analyzer') as self:
    with description('REST API,'):

        with before.all:
            service = Service(CONFIG.service())
            self.process = service.start()
            self.api = client.api.PacketAnalyzersApi(service.client())
            if not check_modules_exists(service.client(), 'packet-analyzer'):
                self.skip()

        with description('invalid HTTP methods,'):
            with description('/packet/analyzers,'):
                with it('returns 405'):
                    expect(lambda: self.api.api_client.call_api('/packet/analyzers', 'PUT')).to(
                        raise_api_exception(405, headers={'Allow': "DELETE, GET, POST"}))

            with description('/packet/analyzer-results,'):
                with it('returns 405'):
                    expect(lambda: self.api.api_client.call_api('/packet/analyzer-results', 'PUT')).to(
                        raise_api_exception(405, headers={'Allow': "DELETE, GET"}))

            with description('/packet/rx-flows,'):
                with it('returns 405'):
                    expect(lambda: self.api.api_client.call_api('/packet/rx-flows', 'PUT')).to(
                        raise_api_exception(405, headers={'Allow': "GET"}))

        with description('list analyzers,'):
            with before.each:
                ana = self.api.create_packet_analyzer(analyzer_model(self.api.api_client))
                expect(ana).to(be_valid_packet_analyzer)
                self.analyzer = ana

            with description('unfiltered,'):
                with it('succeeds'):
                    analyzers = self.api.list_packet_analyzers()
                    expect(analyzers).not_to(be_empty)
                    for ana in analyzers:
                        expect(ana).to(be_valid_packet_analyzer)

            with description('filtered,'):
                with description('by source_id,'):
                    with it('returns an analyzer'):
                        analyzers = self.api.list_packet_analyzers(source_id=self.analyzer.source_id)
                        expect(analyzers).not_to(be_empty)
                        for ana in analyzers:
                            expect(ana).to(be_valid_packet_analyzer)
                        expect([ a for a in analyzers if a.id == self.analyzer.id ]).not_to(be_empty)

                with description('non-existent source_id,'):
                    with it('returns no analyzers'):
                        analyzers = self.api.list_packet_analyzers(source_id='foo')
                        expect(analyzers).to(be_empty)

        with description('get analyzer,'):
            with description('by existing analyzer id,'):
                with before.each:
                    ana = self.api.create_packet_analyzer(analyzer_model(self.api.api_client))
                    expect(ana).to(be_valid_packet_analyzer)
                    self.analyzer = ana

                with it('succeeds'):
                    expect(self.api.get_packet_analyzer(self.analyzer.id)).to(be_valid_packet_analyzer)

            with description('non-existent analyzer,'):
                with it('returns 404'):
                    expect(lambda: self.api.get_packet_analyzer('foo')).to(raise_api_exception(404))

            with description('invalid analyzer id,'):
                with it('returns 404'):
                    expect(lambda: self.api.get_packet_analyzer('foo')).to(raise_api_exception(404))

        with description('create analyzer,'):
            with description('empty source id,'):
                with it('returns 400'):
                    ana = analyzer_model(self.api.api_client)
                    ana.source_id = None
                    expect(lambda: self.api.create_packet_analyzer(ana)).to(raise_api_exception(400))

            with description('non-existent source id,'):
                with it('returns 400'):
                    ana = analyzer_model(self.api.api_client)
                    ana.source_id = 'foo'
                    expect(lambda: self.api.create_packet_analyzer(ana)).to(raise_api_exception(400))

            with description('invalid protocol counter'):
                with it('returns 400'):
                    ana = analyzer_model(self.api.api_client)
                    ana.config.protocol_counters = ['foo']
                    expect(lambda: self.api.create_packet_analyzer(ana)).to(raise_api_exception(400))

            with description('invalid flow counter'):
                with it('returns 400'):
                    ana = analyzer_model(self.api.api_client)
                    ana.config.flow_counters = ['bar']
                    expect(lambda: self.api.create_packet_analyzer(ana)).to(raise_api_exception(400))

        with description('delete analyzer,'):
            with description('by existing analyzer id,'):
                with before.each:
                    ana = self.api.create_packet_analyzer(analyzer_model(self.api.api_client))
                    expect(ana).to(be_valid_packet_analyzer)
                    self.analyzer = ana

                with it('succeeds'):
                    self.api.delete_packet_analyzer(self.analyzer.id)
                    expect(self.api.list_packet_analyzers()).to(be_empty)

            with description('non-existent analyzer id,'):
                with it('succeeds'):
                    self.api.delete_packet_analyzer('foo')

            with description('invalid analyzer id,'):
                with it('returns 404'):
                    expect(lambda: self.api.delete_packet_analyzer("invalid_id")).to(raise_api_exception(404))

        with description('start analyzer,'):
            with before.each:
                ana = self.api.create_packet_analyzer(analyzer_model(self.api.api_client))
                expect(ana).to(be_valid_packet_analyzer)
                self.analyzer = ana

            with description('by existing analyzer id,'):
                with it('succeeds'):
                    result = self.api.start_packet_analyzer(self.analyzer.id)
                    expect(result).to(be_valid_packet_analyzer_result)
                    expect(result.active).to(be_true)

                    ana = self.api.get_packet_analyzer(self.analyzer.id)
                    expect(ana).to(be_valid_packet_analyzer)
                    expect(ana.active).to(be_true)

            with description('non-existent analyzer id,'):
                with it('returns 404'):
                    expect(lambda: self.api.start_packet_analyzer('foo')).to(raise_api_exception(404))

        with description('stop running analyzer,'):
            with before.each:
                ana = self.api.create_packet_analyzer(analyzer_model(self.api.api_client))
                expect(ana).to(be_valid_packet_analyzer)
                self.analyzer = ana
                result = self.api.start_packet_analyzer(self.analyzer.id)
                expect(result).to(be_valid_packet_analyzer_result)
                expect(result.active).to(be_true)

            with description('by analyzer id,'):
                with it('succeeds'):
                    ana = self.api.get_packet_analyzer(self.analyzer.id)
                    expect(ana).to(be_valid_packet_analyzer)
                    expect(ana.active).to(be_true)

                    self.api.stop_packet_analyzer(self.analyzer.id)

                    ana = self.api.get_packet_analyzer(self.analyzer.id)
                    expect(ana).to(be_valid_packet_analyzer)
                    expect(ana.active).to(be_false)

                    results = self.api.list_packet_analyzer_results(analyzer_id=self.analyzer.id)
                    expect(results).not_to(be_empty)
                    for result in results:
                        expect(result).to(be_valid_packet_analyzer_result)
                        expect(result.active).to(be_false)

            with description('restart analyzer, '):
                with it('succeeds'):
                    self.api.stop_packet_analyzer(self.analyzer.id)
                    result = self.api.start_packet_analyzer(self.analyzer.id)
                    expect(result).to(be_valid_packet_analyzer_result)
                    expect(result.active).to(be_true)

                    # We should now have two results: one active, one inactive
                    results = self.api.list_packet_analyzer_results(analyzer_id=self.analyzer.id)
                    expect(results).not_to(be_empty)
                    for result in results:
                        expect(result).to(be_valid_packet_analyzer_result)
                    expect([r for r in results if r.active is True]).not_to(be_empty)
                    expect([r for r in results if r.active is False]).not_to(be_empty)

        with description('reset analyzer,'):
            with before.each:
                ana = self.api.create_packet_analyzer(analyzer_model(self.api.api_client))
                expect(ana).to(be_valid_packet_analyzer)
                self.analyzer = ana

            with description('running,'):
                with it('succeeds'):
                    result1 = self.api.start_packet_analyzer(self.analyzer.id)
                    expect(result1).to(be_valid_packet_analyzer_result)
                    expect(result1.active).to(be_true)

                    result2 = self.api.reset_packet_analyzer(self.analyzer.id)
                    expect(result2).to(be_valid_packet_analyzer_result)
                    expect(result2.active).to(be_true)

                    # Make sure original result is no longer active
                    result3 = self.api.get_packet_analyzer_result(result1.id)
                    expect(result3).to(be_valid_packet_analyzer_result)
                    expect(result3.active).to(be_false)

            with description('stopped,'):
                with it('returns 400'):
                    expect(lambda: self.api.reset_packet_analyzer(self.analyzer.id)).to(raise_api_exception(400))

            with description('invalid,'):
                with it('returns 404'):
                    expect(lambda: self.api.reset_packet_analyzer('baz')).to(raise_api_exception(404))

        with description('list analyzer results,'):
            with before.each:
                ana = self.api.create_packet_analyzer(analyzer_model(self.api.api_client))
                expect(ana).to(be_valid_packet_analyzer)
                self.analyzer = ana
                result = self.api.start_packet_analyzer(self.analyzer.id)
                expect(result).to(be_valid_packet_analyzer_result)

            with description('unfiltered,'):
                with it('succeeds'):
                    results = self.api.list_packet_analyzer_results()
                    expect(results).not_to(be_empty)
                    for result in results:
                        expect(result).to(be_valid_packet_analyzer_result)

            with description('by analyzer id,'):
                with it('succeeds'):
                    results = self.api.list_packet_analyzer_results(analyzer_id=self.analyzer.id)
                    for result in results:
                        expect(result).to(be_valid_packet_analyzer_result)
                    expect([ r for r in results if r.analyzer_id == self.analyzer.id ]).not_to(be_empty)

            with description('non-existent analyzer id,'):
                with it('returns no results'):
                    results = self.api.list_packet_analyzer_results(analyzer_id='foo')
                    expect(results).to(be_empty)

            with description('by source id,'):
                with it('succeeds'):
                    results = self.api.list_packet_analyzer_results(source_id=get_first_port_id(self.api.api_client))
                    expect(results).not_to(be_empty)

            with description('non-existent source id,'):
                with it('returns no results'):
                    results = self.api.list_packet_analyzer_results(source_id='bar')
                    expect(results).to(be_empty)

        with description('list rx flows,'):
            with description('with no flows,'):
                with it('returns empty list'):
                    flows = self.api.list_rx_flows()
                    expect(flows).to(be_empty)

                with it('accepts any id'):
                    flows = self.api.list_rx_flows(analyzer_id='foo')
                    expect(flows).to(be_empty)

        with description('get rx flow, '):
            with description('with invalid id, '):
                with it('returns 404'):
                    expect(lambda: self.api.get_rx_flow('foo')).to(raise_api_exception(404))

        with description('bulk operations,'):
            with description('bulk create,'):
                with description('valid request,'):
                    with it('succeeds'):
                        request = client.models.BulkCreatePacketAnalyzersRequest()
                        request.items = analyzer_models(self.api.api_client)
                        reply = self.api.bulk_create_packet_analyzers(request)
                        expect(reply.items).to(have_len(len(request.items)))
                        for item in reply.items:
                            expect(item).to(be_valid_packet_analyzer)

                with description('invalid requests,'):
                    with it('returns 400 for invalid config'):
                        request = client.models.BulkCreatePacketAnalyzersRequest()
                        request.items = analyzer_models(self.api.api_client)
                        request.items[-1].config.flow_counters = ['invalid']
                        expect(lambda: self.api.bulk_create_packet_analyzers(request)).to(raise_api_exception(400))
                        expect(self.api.list_packet_analyzers()).to(be_empty)

                    with it('returns 404 for invalid id'):
                        request = client.models.BulkCreatePacketAnalyzersRequest()
                        request.items = analyzer_models(self.api.api_client)
                        request.items[-1].id = 'cool::name'
                        expect(lambda: self.api.bulk_create_packet_analyzers(request)).to(raise_api_exception(404))
                        expect(self.api.list_packet_analyzers()).to(be_empty)

            with description('bulk delete,'):
                with before.each:
                    request = client.models.BulkCreatePacketAnalyzersRequest()
                    request.items = analyzer_models(self.api.api_client)
                    reply = self.api.bulk_create_packet_analyzers(request)
                    expect(reply.items).to(have_len(len(request.items)))
                    for item in reply.items:
                        expect(item).to(be_valid_packet_analyzer)

                with description('valid request,'):
                    with it('succeeds'):
                        self.api.bulk_delete_packet_analyzers(
                            client.models.BulkDeletePacketAnalyzersRequest(
                                [ana.id for ana in self.api.list_packet_analyzers()]))
                        expect(self.api.list_packet_analyzers()).to(be_empty)

                with description('invalid requests,'):
                    with it('succeeds with a non-existent id'):
                        self.api.bulk_delete_packet_analyzers(
                            client.models.BulkDeletePacketAnalyzersRequest(
                                [ana.id for ana in self.api.list_packet_analyzers()] + ['foo']))
                        expect(self.api.list_packet_analyzers()).to(be_empty)

                    with it('returns 404 for an invalid id'):
                        expect(lambda: self.api.bulk_delete_packet_analyzers(
                            client.models.BulkDeletePacketAnalyzersRequest(
                                [ana.id for ana in self.api.list_packet_analyzers()] + ['::bar']))).to(
                                    raise_api_exception(404))
                        expect(self.api.list_packet_analyzers()).not_to(be_empty)

            with description('bulk start,'):
                with before.each:
                    request = client.models.BulkCreatePacketAnalyzersRequest()
                    request.items = analyzer_models(self.api.api_client)
                    reply = self.api.bulk_create_packet_analyzers(request)
                    expect(reply.items).to(have_len(len(request.items)))
                    for item in reply.items:
                        expect(item).to(be_valid_packet_analyzer)

                with description('valid request'):
                    with it('succeeds'):
                        reply = self.api.bulk_start_packet_analyzers(
                            client.models.BulkStartPacketAnalyzersRequest(
                                [ana.id for ana in self.api.list_packet_analyzers()]))
                        expect(reply.items).to(have_len(len(self.api.list_packet_analyzers())))
                        for item in reply.items:
                            expect(item).to(be_valid_packet_analyzer_result)
                            expect(item.active).to(be_true)

                with description('invalid requests,'):
                    with it('returns 404 for non-existing id'):
                        expect(lambda: self.api.bulk_start_packet_analyzers(
                            client.models.BulkStartPacketAnalyzersRequest(
                                [ana.id for ana in self.api.list_packet_analyzers()] + ['foo']))).to(
                                    raise_api_exception(404))
                        for ana in self.api.list_packet_analyzers():
                            expect(ana.active).to(be_false)

                    with it('returns 404 for invalid id'):
                        expect(lambda: self.api.bulk_start_packet_analyzers(
                            client.models.BulkStartPacketAnalyzersRequest(
                                [ana.id for ana in self.api.list_packet_analyzers()] + ['::bar']))).to(
                                    raise_api_exception(404))
                        for ana in self.api.list_packet_analyzers():
                            expect(ana.active).to(be_false)

            with description('bulk stop,'):
                with before.each:
                    create_request = client.models.BulkCreatePacketAnalyzersRequest()
                    create_request.items = analyzer_models(self.api.api_client)
                    create_reply = self.api.bulk_create_packet_analyzers(create_request)
                    expect(create_reply.items).to(have_len(len(create_request.items)))
                    for item in create_reply.items:
                        expect(item).to(be_valid_packet_analyzer)
                    start_reply = self.api.bulk_start_packet_analyzers(
                        client.models.BulkStartPacketAnalyzersRequest(
                            [ana.id for ana in create_reply.items]))
                    expect(start_reply.items).to(have_len(len(create_request.items)))
                    for item in start_reply.items:
                        expect(item).to(be_valid_packet_analyzer_result)
                        expect(item.active).to(be_true)

                with description('valid request,'):
                    with it('succeeds'):
                        self.api.bulk_stop_packet_analyzers(
                            client.models.BulkStopPacketAnalyzersRequest(
                                [ana.id for ana in self.api.list_packet_analyzers()]))
                        analyzers = self.api.list_packet_analyzers()
                        expect(analyzers).not_to(be_empty)
                        for ana in analyzers:
                            expect(ana.active).to(be_false)

                with description('invalid requests,'):
                    with it('succeeds with a non-existent id'):
                        self.api.bulk_stop_packet_analyzers(
                            client.models.BulkStopPacketAnalyzersRequest(
                                [ana.id for ana in self.api.list_packet_analyzers()] + ['foo']))
                        analyzers = self.api.list_packet_analyzers()
                        expect(analyzers).not_to(be_empty)
                        for ana in analyzers:
                            expect(ana.active).to(be_false)

                    with it('returns 404 for an invalid id'):
                        expect(lambda: self.api.bulk_stop_packet_analyzers(
                            client.models.BulkStopPacketAnalyzersRequest(
                                [ana.id for ana in self.api.list_packet_analyzers()] + ['::bar']))).to(
                                    raise_api_exception(404))
                        analyzers = self.api.list_packet_analyzers()
                        expect(analyzers).not_to(be_empty)
                        for ana in analyzers:
                            expect(ana.active).to(be_true)


        with after.each:
            try:
                for ana in self.api.list_packet_analyzers():
                    if ana.active:
                        self.api.stop_packet_analyzer(ana.id)
                self.api.delete_packet_analyzers()
            except AttributeError:
                pass
            self.analyzer = None

        with after.all:
            try:
                self.process.terminate()
                self.process.wait()
            except AttributeError:
                pass
