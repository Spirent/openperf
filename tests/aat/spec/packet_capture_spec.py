from mamba import description, before, after
from expects import *
import os

import client.api
import client.models
from common import Config, Service
from common.matcher import (be_valid_packet_capture,
                            be_valid_packet_capture_result,
                            raise_api_exception)


CONFIG = Config(os.path.join(os.path.dirname(__file__),
                             os.environ.get('MAMBA_CONFIG', 'config.yaml')))


def get_first_port_id(api_client):
    ports_api = client.api.PortsApi(api_client)
    ports = ports_api.list_ports()
    expect(ports).not_to(be_empty)
    return ports[0].id


def capture_model(api_client):
    config = client.models.PacketCaptureConfig(mode='buffer', buffer_size=16*1024*1024)
    capture = client.models.PacketCapture()
    capture.source_id = get_first_port_id(api_client)
    capture.config = config

    return capture


with description('Packet Capture,', 'packet_capture') as self:
    with description('REST API,'):

        with before.all:
            service = Service(CONFIG.service())
            self.process = service.start()
            self.api = client.api.PacketCapturesApi(service.client())

        with description('invalid HTTP methods,'):
            with description('/packet/captures,'):
                with it('returns 405'):
                    expect(lambda: self.api.api_client.call_api('/packet/captures', 'PUT')).to(
                        raise_api_exception(405, headers={'Allow': "DELETE, GET, POST"}))

            with description('/packet/capture-results,'):
                with it('returns 405'):
                    expect(lambda: self.api.api_client.call_api('/packet/capture-results', 'PUT')).to(
                        raise_api_exception(405, headers={'Allow': "DELETE, GET"}))

        with description('list captures,'):
            with before.each:
                cap = self.api.create_capture(capture_model(self.api.api_client))
                expect(cap).to(be_valid_packet_capture)
                self.capture = cap

            with description('unfiltered,'):
                with it('succeeds'):
                    captures = self.api.list_captures()
                    expect(captures).not_to(be_empty)
                    for cap in captures:
                        expect(cap).to(be_valid_packet_capture)

            with description('filtered,'):
                with description('by source_id,'):
                    with it('returns an capture'):
                        captures = self.api.list_captures(source_id=self.capture.source_id)
                        expect(captures).not_to(be_empty)
                        for cap in captures:
                            expect(cap).to(be_valid_packet_capture)
                        expect([ cap for cap in captures if cap.id == self.capture.id ]).not_to(be_empty)

                with description('non-existent source_id,'):
                    with it('returns no captures'):
                        captures = self.api.list_captures(source_id='foo')
                        expect(captures).to(be_empty)

        with description('get capture,'):
            with description('by existing capture id,'):
                with before.each:
                    cap = self.api.create_capture(capture_model(self.api.api_client))
                    expect(cap).to(be_valid_packet_capture)
                    self.capture = cap

                with it('succeeds'):
                    expect(self.api.get_capture(self.capture.id)).to(be_valid_packet_capture)

            with description('non-existent capture,'):
                with it('returns 404'):
                    expect(lambda: self.api.get_capture('foo')).to(raise_api_exception(404))

            with description('invalid capture id,'):
                with it('returns 404'):
                    expect(lambda: self.api.get_capture('foo')).to(raise_api_exception(404))

        with description('create capture,'):
            with description('empty source id,'):
                with it('returns 400'):
                    cap = capture_model(self.api.api_client)
                    cap.source_id = None
                    expect(lambda: self.api.create_capture(cap)).to(raise_api_exception(400))

            with description('non-existent source id,'):
                with it('returns 400'):
                    cap = capture_model(self.api.api_client)
                    cap.source_id = 'foo'
                    expect(lambda: self.api.create_capture(cap)).to(raise_api_exception(400))

        with description('delete capture,'):
            with description('by existing capture id,'):
                with before.each:
                    cap = self.api.create_capture(capture_model(self.api.api_client))
                    expect(cap).to(be_valid_packet_capture)
                    self.capture = cap

                with it('succeeds'):
                    self.api.delete_capture(self.capture.id)
                    expect(self.api.list_captures()).to(be_empty)

            with description('non-existent capture id,'):
                with it('succeeds'):
                    self.api.delete_capture('foo')

            with description('invalid capture id,'):
                with it('returns 404'):
                    expect(lambda: self.api.delete_capture("invalid_id")).to(raise_api_exception(404))

        with description('start capture,'):
            with before.each:
                cap = self.api.create_capture(capture_model(self.api.api_client))
                expect(cap).to(be_valid_packet_capture)
                self.capture = cap

            with description('by existing capture id,'):
                with it('succeeds'):
                    result = self.api.start_capture(self.capture.id)
                    expect(result).to(be_valid_packet_capture_result)

            with description('non-existent capture id,'):
                with it('returns 404'):
                    expect(lambda: self.api.start_capture('foo')).to(raise_api_exception(404))

        with description('stop running capture,'):
            with before.each:
                cap = self.api.create_capture(capture_model(self.api.api_client))
                expect(cap).to(be_valid_packet_capture)
                self.capture = cap
                result = self.api.start_capture(self.capture.id)
                expect(result).to(be_valid_packet_capture_result)

            with description('by capture id,'):
                with it('succeeds'):
                    cap = self.api.get_capture(self.capture.id)
                    expect(cap).to(be_valid_packet_capture)
                    expect(cap.active).to(be_true)

                    self.api.stop_capture(self.capture.id)

                    cap = self.api.get_capture(self.capture.id)
                    expect(cap).to(be_valid_packet_capture)
                    expect(cap.active).to(be_false)

        with description('list capture results,'):
            with before.each:
                cap = self.api.create_capture(capture_model(self.api.api_client))
                expect(cap).to(be_valid_packet_capture)
                self.capture = cap
                result = self.api.start_capture(self.capture.id)
                expect(result).to(be_valid_packet_capture_result)

            with description('unfiltered,'):
                with it('succeeds'):
                    results = self.api.list_capture_results()
                    expect(results).not_to(be_empty)
                    for result in results:
                        expect(result).to(be_valid_packet_capture_result)

            with description('by capture id,'):
                with it('succeeds'):
                    results = self.api.list_capture_results(capture_id=self.capture.id)
                    for result in results:
                        expect(result).to(be_valid_packet_capture_result)
                    expect([ r for r in results if r.capture_id == self.capture.id ]).not_to(be_empty)

            with description('non-existent capture id,'):
                with it('returns no results'):
                    results = self.api.list_capture_results(capture_id='foo')
                    expect(results).to(be_empty)

            with description('by source id,'):
                with it('succeeds'):
                    results = self.api.list_capture_results(source_id=get_first_port_id(self.api.api_client))
                    expect(results).not_to(be_empty)

            with description('non-existent source id,'):
                with it('returns no results'):
                    results = self.api.list_capture_results(source_id='bar')
                    expect(results).to(be_empty)

        ###
        # XXX: Fill these in when we actually implement the code....
        ###
        with description('capture packets,'):
            with description('counters,'):
                with _it('TODO'):
                    assert False
            with description('pcap,'):
                with _it('TODO'):
                    assert False
            with description('live,'):
                with _it('TODO'):
                    assert False

        with after.each:
            try:
                for cap in self.api.list_captures():
                    if cap.active:
                        self.api.stop_capture(cap.id)
                self.api.delete_captures()
            except AttributeError:
                pass
            self.capture = None

        with after.all:
            try:
                self.process.terminate()
                self.process.wait()
            except AttributeError:
                pass

