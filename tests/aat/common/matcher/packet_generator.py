from expects import (expect,
                     be_a,
                     be_empty,
                     be_none,
                     be_true,
                     be_above,
                     be_above_or_equal,
                     be_below)
from expects.matchers import Matcher

import client.models


class _be_valid_packet_generator(Matcher):
    def _match(self, generator):
        expect(generator).to(be_a(client.models.PacketGenerator))
        expect(generator.id).not_to(be_empty)
        expect(generator.target_id).not_to(be_empty)
        expect(generator.active).not_to(be_none)
        expect(generator.config).to(be_valid_packet_generator_config)
        return True, ['is valid packet generator']


class _be_valid_packet_generator_config(Matcher):
    def _match(self, config):
        expect(config).to(be_a(client.models.PacketGeneratorConfig))
        if (config.flow_count):
            expect(config.flow_count).to(be_above_or_equal(1))
        expect(config.duration).to(be_valid_traffic_duration)
        expect(config.load).to(be_valid_traffic_load)
        expect(config.traffic).not_to(be_empty)
        for definition in config.traffic:
            expect(definition).to(be_valid_traffic_definition)
        return True, ['is valid packet generator config']


class _be_valid_packet_generator_flow_counters(Matcher):
    def _match(self, counters):
        expect(counters).to(be_a(client.models.PacketGeneratorFlowCounters))
        expect(counters.octets_actual).not_to(be_none)
        expect(counters.packets_actual).not_to(be_none)
        if (counters.packets_actual):
            expect(counters.timestamp_first).not_to(be_none)
            expect(counters.timestamp_last).not_to(be_none)
        return True, ['is valid packet generator flow counter']


class _be_valid_packet_generator_result(Matcher):
    def _match(self, result):
        expect(result).to(be_a(client.models.PacketGeneratorResult))
        expect(result.id).not_to(be_empty)
        expect(result.generator_id).not_to(be_empty)
        expect(result.active).not_to(be_none)
        expect(result.flow_counters).to(be_a(client.models.PacketGeneratorFlowCounters))
        if (result.active):
            expect(result.remaining).to(be_a(client.models.DurationRemainder))
        return True, ['is valid packet generator result']


class _be_valid_transmit_flow(Matcher):
    def _match(self, flow):
        expect(flow).to(be_a(client.models.TxFlow))
        expect(flow.id).not_to(be_empty)
        expect(flow.generator_result_id).not_to(be_empty)
        expect(flow.counters).to(be_valid_packet_generator_flow_counters)
        return True, ['is valid transmit flow']


class _be_valid_traffic_duration(Matcher):
    def _match(self, duration):
        expect(duration).to(be_a(client.models.TrafficDuration))
        has_property = False
        if (duration.continuous):
            has_property = True
        if (duration.frames):
            expect(duration.frames).to(be_above(0))
            has_property = True
        if (duration.time):
            expect(duration.time.value).to(be_above(0))
            expect(duration.time.units).not_to(be_empty)
        expect(has_property).to(be_true)
        return True, ['is valid traffic duration']


class _be_valid_traffic_load(Matcher):
    def _match(self, load):
        expect(load).to(be_a(client.models.TrafficLoad))
        if (load.burst_size):
            expect(load.burst_size).to(be_above(0))
            expect(load.burst_size).to(be_below(65536))
        expect(load.rate).not_to(be_none)
        expect(load.rate.period).not_to(be_none)
        expect(load.rate.value).to(be_above(0))
        expect(load.units).not_to(be_empty)
        return True, ['is valid traffic load']


class _be_valid_traffic_definition(Matcher):
    def _match(self, definition):
        expect(definition).to(be_a(client.models.TrafficDefinition))
        expect(definition.packet).to(be_valid_traffic_packet_template)
        expect(definition.length).to(be_valid_traffic_length)
        if (definition.signature):
            expect(definition.signature).to(be_a(client.models.SpirentSignature))
        if (definition.weight):
            expect(definition.weight).to(be_above(0))
        return True, ['is valid traffic definition']


class _be_valid_traffic_length(Matcher):
    def _match(self, length):
        expect(length).to(be_a(client.models.TrafficLength))
        has_property = False
        if (length.fixed):
            expect(length.fixed).to(be_above_or_equal(64))
            has_property = True
        if (length.list):
            expect(length.list).not_to(be_empty)
            for l in length.list:
                expect(l).to(be_above_or_equal(64))
            has_property = True
        if (length.sequence):
            expect(length.sequence).to(be_a(client.models.TrafficLengthSequence))
            expect(length.sequence.count).to(be_above(0))
            expect(length.sequence.start).to(be_above_or_equal(64))
            has_property = True
        expect(has_property).to(be_true)
        return True, ['is valid traffic length']


class _be_valid_traffic_packet_template(Matcher):
    def _match(self, template):
        expect(template).to(be_a(client.models.TrafficPacketTemplate))
        expect(template.protocols).not_to(be_empty)
        for protocol in template.protocols:
            expect(protocol).to(be_a(client.models.TrafficProtocol))
        return True, ['is valid traffic packet template']


be_valid_packet_generator = _be_valid_packet_generator()
be_valid_packet_generator_config = _be_valid_packet_generator_config()
be_valid_packet_generator_flow_counters = _be_valid_packet_generator_flow_counters()
be_valid_packet_generator_result = _be_valid_packet_generator_result()
be_valid_transmit_flow = _be_valid_transmit_flow()
be_valid_traffic_definition = _be_valid_traffic_definition()
be_valid_traffic_duration = _be_valid_traffic_duration()
be_valid_traffic_length = _be_valid_traffic_length()
be_valid_traffic_load = _be_valid_traffic_load()
be_valid_traffic_packet_template = _be_valid_traffic_packet_template()
