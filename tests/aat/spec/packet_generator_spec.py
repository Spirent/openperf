from mamba import description, before, after
from expects import *
import os

import client.api
import client.models
from common import Config, Service
from common.matcher import (be_valid_packet_generator,
                            be_valid_packet_generator_result,
                            be_valid_transmit_flow,
                            raise_api_exception)


CONFIG = Config(os.path.join(os.path.dirname(__file__),
                             os.environ.get('MAMBA_CONFIG', 'config.yaml')))


def get_first_port_id(api_client):
    ports_api = client.api.PortsApi(api_client)
    ports = ports_api.list_ports()
    expect(ports).not_to(be_empty)
    return ports[0].id


def default_traffic_duration():
    duration = client.models.TrafficDuration()
    duration.continuous = True
    return duration


def default_traffic_load():
    rate = client.models.TrafficLoadRate()
    rate.period = "second"
    rate.value = 10

    load = client.models.TrafficLoad()
    load.rate = rate
    load.units = "frames"

    return load

def default_traffic_packet_template():
    eth = client.models.PacketProtocolEthernet()
    eth.source = "10:94:00:01:aa:bb"
    eth.destination = "10:94:00.02:cc:dd"

    proto_eth = client.models.TrafficProtocol()
    proto_eth.ethernet = eth

    ip = client.models.PacketProtocolIpv4()
    ip.source = "198.18.15.10"
    ip.destination = "198.18.15.20"

    proto_ip = client.models.TrafficProtocol()
    proto_ip.ipv4 = ip

    udp = client.models.PacketProtocolUdp()
    proto_udp = client.models.TrafficProtocol()
    proto_udp.udp = udp

    template = client.models.TrafficPacketTemplate()
    template.protocols = [proto_eth, proto_ip, proto_udp]
    return template


def default_traffic_packet_template_with_modifiers(permute_flag=None):
    mac_seq = client.models.TrafficProtocolMacModifierSequence()
    mac_seq.count = 10
    mac_seq.start = "00.00.01.00.00.01"
    mac_seq_mod = client.models.TrafficProtocolMacModifier(sequence=mac_seq)
    eth_modifier = client.models.TrafficProtocolModifier(name='destination',
                                                         permute=permute_flag if permute_flag else False,
                                                         mac=mac_seq_mod)

    addr_src_seq = client.models.TrafficProtocolIpv4ModifierSequence()
    addr_src_seq.count = 10
    addr_src_seq.start = '198.18.15.1'
    addr_src_mod = client.models.TrafficProtocolIpv4Modifier(sequence=addr_src_seq)
    ipv4_src_mod = client.models.TrafficProtocolModifier(name='source',
                                                         permute=permute_flag if permute_flag else False,
                                                         ipv4=addr_src_mod)

    addr_dst_seq = client.models.TrafficProtocolIpv4ModifierSequence()
    addr_dst_seq.count = 10
    addr_dst_seq.start = '198.18.15.1'
    addr_dst_mod = client.models.TrafficProtocolIpv4Modifier(sequence=addr_dst_seq)
    ipv4_dst_mod = client.models.TrafficProtocolModifier(name='destination',
                                                         permute=permute_flag if permute_flag else False,
                                                         ipv4=addr_dst_mod)


    template = default_traffic_packet_template()
    template.protocols[0].modifiers = client.models.TrafficProtocolModifiers(
        items=[eth_modifier])
    template.protocols[1].modifiers = client.models.TrafficProtocolModifiers(
        items=[ipv4_src_mod, ipv4_dst_mod])

    return template


def default_traffic_length():
    length = client.models.TrafficLength()
    length.fixed = 128
    return length


def default_traffic_definition():
    definition = client.models.TrafficDefinition()
    definition.packet = default_traffic_packet_template()
    definition.length = default_traffic_length()
    return definition


def generator_model(api_client, duration = None, load = None, traffic = None):
    config = client.models.PacketGeneratorConfig()
    config.duration = duration if duration else default_traffic_duration()
    config.load = load if load else default_traffic_load()
    config.traffic = [traffic if traffic else default_traffic_definition()]

    gen = client.models.PacketGenerator()
    gen.target_id = get_first_port_id(api_client)
    gen.config = config

    return gen


with description('Packet Generator,', 'packet_generator') as self:
    with description('REST API'):

        with before.all:
            service = Service(CONFIG.service())
            self.process = service.start()
            self.api = client.api.PacketGeneratorsApi(service.client())

        with description('invalid HTTP methods,'):
            with description('/packet/generators,'):
                with it('returns 405'):
                    expect(lambda: self.api.api_client.call_api('/packet/generators', 'PUT')).to(
                        raise_api_exception(405, headers={'Allow': "DELETE, GET, POST"}))

            with description('/packet/generator-results,'):
                with it('returns 405'):
                    expect(lambda: self.api.api_client.call_api('/packet/generator-results', 'PUT')).to(
                        raise_api_exception(405, headers={'Allow': "DELETE, GET"}))

            with description('/packet/tx-flows,'):
                with it('returns 405'):
                    expect(lambda: self.api.api_client.call_api('/packet/tx-flows', 'PUT')).to(
                        raise_api_exception(405, headers={'Allow': "GET"}))

        with description('list generators,'):
            with before.each:
                gen = self.api.create_generator(generator_model(self.api.api_client))
                expect(gen).to(be_valid_packet_generator)
                self.generator = gen

            with description('unfiltered,'):
                with it('succeeds'):
                    generators = self.api.list_generators()
                    expect(generators).not_to(be_empty)
                    for gen in generators:
                        expect(gen).to(be_valid_packet_generator)

            with description('filtered,'):
                with description('by target_id,'):
                    with it('returns an generator'):
                        generators = self.api.list_generators(target_id=self.generator.target_id)
                        expect(generators).not_to(be_empty)
                        for ana in generators:
                            expect(ana).to(be_valid_packet_generator)
                        expect([ a for a in generators if a.id == self.generator.id ]).not_to(be_empty)

                with description('non-existent target_id,'):
                    with it('returns no generators'):
                        generators = self.api.list_generators(target_id='foo')
                        expect(generators).to(be_empty)

        with description('get generator,'):
            with description('by existing generator id,'):
                with before.each:
                    gen = self.api.create_generator(generator_model(self.api.api_client))
                    expect(gen).to(be_valid_packet_generator)
                    self.generator = gen

                with it('succeeds'):
                    expect(self.api.get_generator(self.generator.id)).to(be_valid_packet_generator)

            with description('non-existent generator,'):
                with it('returns 404'):
                    expect(lambda: self.api.get_generator('foo')).to(raise_api_exception(404))

            with description('invalid generator id,'):
                with it('returns 404'):
                    expect(lambda: self.api.get_generator(':bar:')).to(raise_api_exception(404))

        with description('create generator,'):
            with description('valid config,'):
                with description('without modifiers,'):
                    with it('succeeds'):
                        gen = generator_model(self.api.api_client)
                        result = self.api.create_generator(gen)
                        expect(result).to(be_valid_packet_generator)

                with description('with modifiers,'):
                    with it('succeeds'):
                        template = default_traffic_packet_template_with_modifiers()
                        gen = generator_model(self.api.api_client)
                        gen.config.traffic[0].packet = template
                        result = self.api.create_generator(gen)
                        expect(result).to(be_valid_packet_generator)

                with description('with permuted modifiers,'):
                    with it('succeeds'):
                        template = default_traffic_packet_template_with_modifiers(permute_flag=True)
                        gen = generator_model(self.api.api_client)
                        gen.config.traffic[0].packet = template
                        result = self.api.create_generator(gen)
                        expect(result).to(be_valid_packet_generator)

                with description('with signatures enabled,'):
                    with it('succeeds'):
                        gen = generator_model(self.api.api_client)
                        gen.config.traffic[0].signature = client.models.SpirentSignature(
                            stream_id=1, latency='start_of_frame')
                        result = self.api.create_generator(gen)
                        expect(result).to(be_valid_packet_generator)

            with description('invalid config'):
                with description('empty target id,'):
                    with it('returns 400'):
                        gen = generator_model(self.api.api_client)
                        gen.target_id = None
                        expect(lambda: self.api.create_generator(gen)).to(raise_api_exception(400))

                with description('non-existent target id,'):
                    with it('returns 400'):
                        gen = generator_model(self.api.api_client)
                        gen.target_id = 'foo'
                        expect(lambda: self.api.create_generator(gen)).to(raise_api_exception(400))

                with description('invalid ordering'):
                    with it('returns 400'):
                        gen = generator_model(self.api.api_client)
                        gen.config.order = 'foo'
                        expect(lambda: self.api.create_generator(gen)).to(raise_api_exception(400))

                with description('invalid load,'):
                    with description('invalid schema,'):
                        with it('returns 400'):
                            gen = generator_model(self.api.api_client)
                            gen.config.load.rate = -1
                            expect(lambda: self.api.create_generator(gen)).to(raise_api_exception(400))

                with description('invalid rate,'):
                    with it('returns 400'):
                        gen = generator_model(self.api.api_client)
                        gen.config.load.rate.value = -1
                        expect(lambda: self.api.create_generator(gen)).to(raise_api_exception(400))

                with description('invalid duration,'):
                    with description('empty duration object,'):
                        with it('returns 400'):
                            gen = generator_model(self.api.api_client)
                            gen.config.duration = client.models.TrafficDuration()
                            expect(lambda: self.api.create_generator(gen)).to(raise_api_exception(400))

                    with description('negative frame count,'):
                        with it('returns 400'):
                            duration = client.models.TrafficDuration()
                            duration.frames = -1;
                            gen = generator_model(self.api.api_client)
                            gen.config.duration = duration
                            expect(lambda: self.api.create_generator(gen)).to(raise_api_exception(400))

                    with description('invalid time object,'):
                        with description('negative time,'):
                            with it('returns 400'):
                                time = client.models.TrafficDurationTime()
                                time.value = -1;
                                time.units = "seconds"
                                duration = client.models.TrafficDuration()
                                duration.time = time
                                gen = generator_model(self.api.api_client)
                                gen.config.duration = duration
                                expect(lambda: self.api.create_generator(gen)).to(raise_api_exception(400))

                        with description('bogus units,'):
                            with it('returns 400'):
                                time = client.models.TrafficDurationTime()
                                time.value = 10;
                                time.units = "foobars"
                                duration = client.models.TrafficDuration()
                                duration.time = time
                                gen = generator_model(self.api.api_client)
                                gen.config.duration = duration
                                expect(lambda: self.api.create_generator(gen)).to(raise_api_exception(400))

                with description('invalid traffic definition,'):
                    with description('no traffic definition,'):
                        with it('returns 400'):
                            gen = generator_model(self.api.api_client)
                            gen.config.traffic = []
                            expect(lambda: self.api.create_generator(gen)).to(raise_api_exception(400))

                    with description('invalid packet,'):
                        with description('invalid modifier tie,'):
                            with it('returns 400'):
                                gen = generator_model(self.api.api_client)
                                gen.config.traffic[0].packet.modifier_tie = 'foo'
                                expect(lambda: self.api.create_generator(gen)).to(raise_api_exception(400))

                        with description('invalid address,'):
                            with it('returns 400'):
                                gen = generator_model(self.api.api_client)
                                gen.config.traffic[0].packet.protocols[0].ethernet.source = 'foo'
                                expect(lambda: self.api.create_generator(gen)).to(raise_api_exception(400))

                    with description('invalid length,'):
                        with description('invalid fixed length,'):
                            with it('returns 400'):
                                length = client.models.TrafficLength()
                                length.fixed = 16
                                gen = generator_model(self.api.api_client)
                                gen.config.traffic[0].length = length
                                expect(lambda: self.api.create_generator(gen)).to(raise_api_exception(400))

                        with description('invalid list length,'):
                            with it('returns 400'):
                                length = client.models.TrafficLength()
                                length.list = [128, 256, 512, 0]
                                gen = generator_model(self.api.api_client)
                                gen.config.traffic[0].length = length
                                expect(lambda: self.api.create_generator(gen)).to(raise_api_exception(400))

                        with description('invalid sequence length,'):
                            with description('invalid count,'):
                                with it('returns 400'):
                                    seq = client.models.TrafficLengthSequence()
                                    seq.count = 0
                                    seq.start = 128
                                    length = client.models.TrafficLength()
                                    length.sequence = seq
                                    gen = generator_model(self.api.api_client)
                                    gen.config.traffic[0].length = length
                                    expect(lambda: self.api.create_generator(gen)).to(raise_api_exception(400))

                            with description('invalid start,'):
                                with it('returns 400'):
                                    seq = client.models.TrafficLengthSequence()
                                    seq.count = 10
                                    seq.start = 0
                                    length = client.models.TrafficLength()
                                    length.sequence = seq
                                    gen = generator_model(self.api.api_client)
                                    gen.config.traffic[0].length = length
                                    expect(lambda: self.api.create_generator(gen)).to(raise_api_exception(400))

                        with description('invalid weight,'):
                            with it('returns 400'):
                                gen = generator_model(self.api.api_client)
                                gen.config.traffic[0].weight = -1
                                expect(lambda: self.api.create_generator(gen)).to(raise_api_exception(400))

                    with description('invalid modifiers,'):
                        with description('too many flows,'):
                            with it('returns 400'):
                                # total flows = 65536^3 which exceeds our flow limit of (1 << 48) - 1
                                template = default_traffic_packet_template_with_modifiers()
                                template.protocols[0].modifiers.items[0].mac.sequence.count = 65536
                                template.protocols[1].modifiers.items[0].ipv4.sequence.count = 65536
                                template.protocols[1].modifiers.items[1].ipv4.sequence.count = 65536
                                template.protocols[1].modifiers.tie = 'cartesian'
                                template.modifier_tie = 'cartesian'
                                gen = generator_model(self.api.api_client)
                                gen.config.traffic[0].packet = template
                                expect(lambda: self.api.create_generator(gen)).to(raise_api_exception(400))

                        with description('too many signature flows,'):
                            with it('returns 400'):
                                # total flows = 256^2 which exceeds our signature flow limit of 64k - 1
                                template = default_traffic_packet_template_with_modifiers()
                                template.protocols[1].modifiers.items[0].ipv4.sequence.count = 256
                                template.protocols[1].modifiers.items[1].ipv4.sequence.count = 256
                                template.protocols[1].modifiers.tie = 'cartesian'
                                gen = generator_model(self.api.api_client)
                                gen.config.traffic[0].packet = template
                                gen.config.traffic[0].signature = client.models.SpirentSignature(
                                    stream_id=1, latency='start_of_frame')
                                expect(lambda: self.api.create_generator(gen)).to(raise_api_exception(400))

        with description('delete generator,'):
            with description('by existing generator id,'):
                with before.each:
                    gen = self.api.create_generator(generator_model(self.api.api_client))
                    expect(gen).to(be_valid_packet_generator)
                    self.generator = gen

                with it('succeeds'):
                    self.api.delete_generator(self.generator.id)
                    expect(self.api.list_generators()).to(be_empty)

            with description('non-existent generator id,'):
                with it('succeeds'):
                    self.api.delete_generator('foo')

            with description('invalid generator id,'):
                with it('returns 404'):
                    expect(lambda: self.api.delete_generator("invalid_id")).to(raise_api_exception(404))

        with description('start generator,'):
            with before.each:
                gen = self.api.create_generator(generator_model(self.api.api_client))
                expect(gen).to(be_valid_packet_generator)
                self.generator = gen

            with description('by existing generator id,'):
                with it('succeeds'):
                    result = self.api.start_generator(self.generator.id)
                    expect(result).to(be_valid_packet_generator_result)

            with description('non-existent generator id,'):
                with it('returns 404'):
                    expect(lambda: self.api.start_generator('foo')).to(raise_api_exception(404))

        with description('stop running generator,'):
            with before.each:
                gen = self.api.create_generator(generator_model(self.api.api_client))
                expect(gen).to(be_valid_packet_generator)
                self.generator = gen
                result = self.api.start_generator(self.generator.id)
                expect(result).to(be_valid_packet_generator_result)

            with description('by generator id,'):
                with it('succeeds'):
                    gen = self.api.get_generator(self.generator.id)
                    expect(gen).to(be_valid_packet_generator)
                    expect(gen.active).to(be_true)

                    self.api.stop_generator(self.generator.id)

                    gen = self.api.get_generator(self.generator.id)
                    expect(gen).to(be_valid_packet_generator)
                    expect(gen.active).to(be_false)

        with description('list generator results,'):
            with before.each:
                gen = self.api.create_generator(generator_model(self.api.api_client))
                expect(gen).to(be_valid_packet_generator)
                self.generator = gen
                result = self.api.start_generator(self.generator.id)
                expect(result).to(be_valid_packet_generator_result)

            with description('unfiltered,'):
                with it('succeeds'):
                    results = self.api.list_generator_results()
                    expect(results).not_to(be_empty)
                    for result in results:
                        expect(result).to(be_valid_packet_generator_result)

            with description('by generator id,'):
                with it('succeeds'):
                    results = self.api.list_generator_results(generator_id=self.generator.id)
                    for result in results:
                        expect(result).to(be_valid_packet_generator_result)
                    expect([ r for r in results if r.generator_id == self.generator.id ]).not_to(be_empty)

            with description('non-existent generator id,'):
                with it('returns no results'):
                    results = self.api.list_generator_results(generator_id='foo')
                    expect(results).to(be_empty)

            with description('by target id,'):
                with it('succeeds'):
                    results = self.api.list_generator_results(target_id=get_first_port_id(self.api.api_client))
                    expect(results).not_to(be_empty)

            with description('non-existent target id,'):
                with it('returns no results'):
                    results = self.api.list_generator_results(target_id='bar')
                    expect(results).to(be_empty)

        with description('list tx flows,'):
            with before.each:
                gen = self.api.create_generator(generator_model(self.api.api_client))
                expect(gen).to(be_valid_packet_generator)
                self.generator = gen
                result = self.api.start_generator(self.generator.id)
                expect(result).to(be_valid_packet_generator_result)
                self.result = result

            with description('unfiltered,'):
                with it('succeeds'):
                    flows = self.api.list_tx_flows()
                    expect(flows).not_to(be_empty)
                    for flow in flows:
                        expect(flow).to(be_valid_transmit_flow)

            with description('filtered,'):
                with description('by target_id,'):
                    with it('returns tx flows'):
                        flows = self.api.list_tx_flows(target_id=self.generator.target_id)
                        expect(flows).not_to(be_empty)
                        for flow in flows:
                            expect(flow).to(be_valid_transmit_flow)
                        expect([f for f in flows if flow.generator_result_id == self.result.id]).not_to(be_empty)

                with description('non-existent target_id,'):
                    with it('returns no flows'):
                        flows = self.api.list_generators(target_id='foo')
                        expect(flows).to(be_empty)

                with description('by generator_id,'):
                    with it('returns tx flows'):
                        flows = self.api.list_tx_flows(generator_id=self.generator.id)
                        expect(flows).not_to(be_empty)
                        for flow in flows:
                            expect(flow).to(be_valid_transmit_flow)
                            # Get generator result of tx flow
                            result = self.api.get_generator_result(id=flow.generator_result_id)
                            expect(result).to(be_valid_packet_generator_result)
                            # Result generator id should match self generator id
                            expect(result.generator_id == self.generator.id).to(be_true)

                with description('non-existent generator_id,'):
                    with it('returns no flows'):
                        flows = self.api.list_generators(target_id='bar')
                        expect(flows).to(be_empty)

        with description('get tx flow,'):
            with before.each:
                gen = self.api.create_generator(generator_model(self.api.api_client))
                expect(gen).to(be_valid_packet_generator)
                self.generator = gen
                result = self.api.start_generator(self.generator.id)
                expect(result).to(be_valid_packet_generator_result)
                self.result = result

            with description('by flow id,'):
                with it('returns tx flow'):
                    result = self.api.get_generator_result(self.result.id)
                    for flow_id in result.flows:
                        flow = self.api.get_tx_flow(flow_id)
                        expect(flow).to(be_valid_transmit_flow)

            with description('non-existent id,'):
                with it('returns 404'):
                    expect(lambda: self.api.get_tx_flow('foo')).to(raise_api_exception(404))

            with description('invalid generator id,'):
                with it('returns 404'):
                    expect(lambda: self.api.get_generator(':bar:')).to(raise_api_exception(404))

        with after.each:
            try:
                for ana in self.api.list_generators():
                    if ana.active:
                        self.api.stop_generator(ana.id)
                self.api.delete_generators()
            except AttributeError:
                pass
            self.generator = None
            self.result = None

        with after.all:
            try:
                self.process.terminate()
                self.process.wait()
            except AttributeError:
                pass
