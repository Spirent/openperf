from expects import expect, be_a, be_empty, be_none
from expects.matchers import Matcher

import client.models

class _be_valid_memory_generator(Matcher):
    def _match(self, g7r):
        expect(g7r).to(be_a(client.models.MemoryGenerator))
        expect(g7r.id).not_to(be_empty)
        expect(g7r.running).to(be_a(bool))
        expect(g7r.config).to(be_a(client.models.MemoryGeneratorConfig))
        return True, ['is valid Memory Generator']

class _be_valid_memory_generator_config(Matcher):
    def _match(self, conf):
        expect(conf).to(be_a(client.models.MemoryGeneratorConfig))
        return True, ['is valid Memory Generator Configuration']

class _be_valid_memory_generator_stats(Matcher):
    def _match(self, stats):
        expect(stats).to(be_a(client.models.MemoryGeneratorStats))
        expect(stats.ops_target).to(be_a(int))
        expect(stats.ops_actual).to(be_a(int))
        expect(stats.bytes_target).to(be_a(int))
        expect(stats.bytes_actual).to(be_a(int))
        expect(stats.io_errors).to(be_a(int))
        expect(stats.latency).to(be_a(int))
        expect(stats.latency_min).to(be_a(int))
        expect(stats.latency_max).to(be_a(int))
        return True, ['is valid Memory Generator Stats']

class _be_valid_memory_generator_result(Matcher):
    def _match(self, result):
        expect(result).to(be_a(client.models.MemoryGeneratorResult))
        expect(result.id).not_to(be_empty)
        expect(result.generator_id).not_to(be_empty)
        expect(result.active).to(be_a(bool))
        #expect(result.timestamp).to(be_a(datetime))
        expect(result.read).to(be_a(client.models.MemoryGeneratorStats))
        expect(result.write).to(be_a(client.models.MemoryGeneratorStats))
        return True, ['is valid Memory Generator Result']

class _be_valid_memory_info(Matcher):
    def _match(self, info):
        expect(info).to(be_a(client.models.MemoryInfoResult))
        expect(info.free_memory).to(be_a(int))
        expect(info.total_memory).to(be_a(int))
        return True, ['is valid Memory Info']

be_valid_memory_info = _be_valid_memory_info()
be_valid_memory_generator = _be_valid_memory_generator()
be_valid_memory_generator_result = _be_valid_memory_generator_result()
