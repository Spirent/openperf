from expects import expect, be_a, be_empty, be_none, be_above
from expects.matchers import Matcher

from common.matcher import be_valid_dynamic_results

import client.models


class _be_valid_tvlp_configuration(Matcher):
    def _match(self, config):
        expect(config).to(be_a(client.models.TvlpConfiguration))
        expect(config.id).not_to(be_empty)
        expect(config.state).not_to(be_none)
        expect(config.time).not_to(be_none)
        expect(config.error).to(be_none)
        expect(config.profile).to(be_a(client.models.TvlpProfile))
        return True, ['is valid TVLP configuration']


class _be_valid_block_tvlp_profile(Matcher):
    def _match(self, profile):
        expect(profile).to(be_a(client.models.TvlpProfileBlock))
        expect(profile.series).not_to(be_empty)
        for conf in profile.series:
            expect(conf).to(be_a(client.models.TvlpProfileBlockSeries))
            expect(conf.length).not_to(be_none)
            expect(conf.length).to(be_above(0))
            expect(conf.resource_id).not_to(be_none)
            expect(conf.config).to(be_a(client.models.BlockGeneratorConfig))
        return True, ['is valid Block TVLP profile']


class _be_valid_memory_tvlp_profile(Matcher):
    def _match(self, profile):
        expect(profile).to(be_a(client.models.TvlpProfileMemory))
        expect(profile.series).not_to(be_empty)
        for conf in profile.series:
            expect(conf).to(be_a(client.models.TvlpProfileMemorySeries))
            expect(conf.length).not_to(be_none)
            expect(conf.length).to(be_above(0))
            expect(conf.config).to(be_a(client.models.MemoryGeneratorConfig))
        return True, ['is valid Memory TVLP profile']


class _be_valid_cpu_tvlp_profile(Matcher):
    def _match(self, profile):
        expect(profile).to(be_a(client.models.TvlpProfileCpu))
        expect(profile.series).not_to(be_empty)
        for conf in profile.series:
            expect(conf).to(be_a(client.models.TvlpProfileCpuSeries))
            expect(conf.length).not_to(be_none)
            expect(conf.length).to(be_above(0))
            expect(conf.config).to(be_a(client.models.CpuGeneratorConfig))
        return True, ['is valid Cpu TVLP profile']


class _be_valid_packet_tvlp_profile(Matcher):
    def _match(self, profile):
        expect(profile).to(be_a(client.models.TvlpProfilePacket))
        expect(profile.series).not_to(be_empty)
        for conf in profile.series:
            expect(conf).to(be_a(client.models.TvlpProfilePacketSeries))
            expect(conf.length).not_to(be_none)
            expect(conf.length).to(be_above(0))
            expect(conf.target_id).not_to(be_none)
            expect(conf.config).to(be_a(client.models.PacketGeneratorConfig))
        return True, ['is valid Packet TVLP profile']


class _be_valid_tvlp_result(Matcher):
    def _match(self, result):
        expect(result).to(be_a(client.models.TvlpResult))
        expect(result.id).not_to(be_none)
        expect(result.tvlp_id).not_to(be_none)
        for prof in [result.memory, result.block, result.cpu]:
            if prof == None: continue
            for gen_result in prof:
                expect(gen_result.dynamic_results).to(be_valid_dynamic_results)
        return True, ['is valid TVLP result']


be_valid_tvlp_configuration = _be_valid_tvlp_configuration()
be_valid_block_tvlp_profile = _be_valid_block_tvlp_profile()
be_valid_memory_tvlp_profile = _be_valid_memory_tvlp_profile()
be_valid_cpu_tvlp_profile = _be_valid_cpu_tvlp_profile()
be_valid_packet_tvlp_profile = _be_valid_packet_tvlp_profile()
be_valid_tvlp_result = _be_valid_tvlp_result()
