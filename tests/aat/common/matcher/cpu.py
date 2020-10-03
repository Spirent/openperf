from expects import expect, be_a, be_empty, be_none
from expects.matchers import Matcher
import client.models

class _be_valid_cpu_info(Matcher):
    def _match(self, info):
        expect(info).to(be_a(client.models.CpuInfoResult))
        expect(info.cores).to(be_a(int))
        expect(info.cache_line_size).to(be_a(int))
        expect(info.architecture).to(be_a(str))
        return True, ['is valid CPU Info']

class _be_valid_cpu_generator(Matcher):
    def _match(self, g7r):
        expect(g7r).to(be_a(client.models.CpuGenerator))
        expect(g7r.id).not_to(be_empty)
        expect(g7r.running).to(be_a(bool))
        expect(g7r.config).to(be_valid_cpu_generator_config)
        return True, ['is valid CPU Generator']

class _be_valid_cpu_generator_config(Matcher):
    def _match(self, conf):
        expect(conf).to(be_a(client.models.CpuGeneratorConfig))
        expect(conf.cores).to(be_a(list))
        for c in conf.cores:
            expect(c).to(be_valid_cpu_generator_core_config)
        return True, ['is valid CPU Generator Configuration']

class _be_valid_cpu_generator_core_config(Matcher):
    def _match(self, conf):
        expect(conf).to(be_a(client.models.CpuGeneratorCoreConfig))
        expect(conf.utilization).to(be_a(float))
        expect(conf.targets).to(be_a(list))
        for t in conf.targets:
            expect(t).to(be_valid_cpu_generator_core_config_targets)
        return True, ['is valid CPU Generator Core Configuration']

class _be_valid_cpu_generator_core_config_targets(Matcher):
    def _match(self, target):
        expect(target).to(be_a(client.models.CpuGeneratorCoreConfigTargets))
        expect(target.instruction_set).to(be_a(str))
        expect(target.data_type).to(be_a(str))
        expect(target.weight).to(be_a(int))
        return True, ['is valid CPU Generator Core Targets Configuration']

class _be_valid_cpu_generator_result(Matcher):
    def _match(self, result):
        expect(result).to(be_a(client.models.CpuGeneratorResult))
        expect(result.id).not_to(be_empty)
        expect(result.generator_id).not_to(be_empty)
        expect(result.active).to(be_a(bool))
        expect(result.timestamp_first).not_to(be_none)
        expect(result.timestamp_last).not_to(be_none)
        expect(result.stats).to(be_a(client.models.CpuGeneratorStats))
        return True, ['is valid CPU Generator Result']

be_valid_cpu_info = _be_valid_cpu_info()
be_valid_cpu_generator = _be_valid_cpu_generator()
be_valid_cpu_generator_config = _be_valid_cpu_generator_config()
be_valid_cpu_generator_core_config = _be_valid_cpu_generator_core_config()
be_valid_cpu_generator_core_config_targets = _be_valid_cpu_generator_core_config_targets()
be_valid_cpu_generator_result = _be_valid_cpu_generator_result()
