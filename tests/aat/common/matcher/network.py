from expects import expect, be_a, be_empty, be_none, be_below_or_equal
from expects.matchers import Matcher

import client.models


class _be_valid_network_server(Matcher):
    def _match(self, g7r):
        expect(g7r).to(be_a(client.models.NetworkServer))
        expect(g7r.id).not_to(be_empty)
        expect(g7r.port).to(be_a(int))
        expect(g7r.protocol).not_to(be_empty)
        return True, ['is valid Network Server']

class _be_valid_network_generator(Matcher):
    def _match(self, g7r):
        expect(g7r).to(be_a(client.models.NetworkGenerator))
        expect(g7r.id).not_to(be_empty)
        expect(g7r.running).to(be_a(bool))
        expect(g7r.config).to(be_a(client.models.NetworkGeneratorConfig))
        return True, ['is valid Network Generator']

class _be_valid_network_generator_stats(Matcher):
    def _match(self, stats):
        expect(stats).to(be_a(client.models.NetworkGeneratorStats))
        expect(stats.ops_target).to(be_a(int))
        expect(stats.ops_actual).to(be_a(int))
        expect(stats.bytes_target).to(be_a(int))
        expect(stats.bytes_actual).to(be_a(int))
        expect(stats.io_errors).to(be_a(int))
        expect(stats.latency_total).to(be_a(int))
        return True, ['is valid Network Generator Stats']

class _be_valid_network_generator_connection_stats(Matcher):
    def _match(self, stats):
        expect(stats).to(be_a(client.models.NetworkGeneratorConnectionStats))
        expect(stats.attempted).to(be_a(int))
        expect(stats.successful).to(be_a(int))
        expect(stats.errors).to(be_a(int))
        expect(stats.closed).to(be_a(int))
        return True, ['is valid Network Generator Connection Stats']


class _be_valid_network_generator_result(Matcher):
    def _match(self, result):
        expect(result).to(be_a(client.models.NetworkGeneratorResult))
        expect(result.id).not_to(be_empty)
        expect(result.generator_id).not_to(be_empty)
        expect(result.active).to(be_a(bool))
        expect(result.timestamp_first).not_to(be_none)
        expect(result.timestamp_last).not_to(be_none)
        expect(result.timestamp_first).to(be_below_or_equal(result.timestamp_last))
        expect(result.read).to(be_a(client.models.NetworkGeneratorStats))
        expect(result.read).to(_be_valid_network_generator_stats())
        expect(result.write).to(be_a(client.models.NetworkGeneratorStats))
        expect(result.write).to(_be_valid_network_generator_stats())
        return True, ['is valid Network Generator Result']

be_valid_network_server = _be_valid_network_server()
be_valid_network_generator = _be_valid_network_generator()
be_valid_network_generator_result = _be_valid_network_generator_result()
