from expects import expect, be_a, be_empty, be_none
from expects.matchers import Matcher

import client.models


class _be_valid_packet_analyzer(Matcher):
    def _match(self, analyzer):
        expect(analyzer).to(be_a(client.models.Analyzer))
        expect(analyzer.id).not_to(be_empty)
        expect(analyzer.source_id).not_to(be_empty)
        expect(analyzer.active).not_to(be_none)
        expect(analyzer.config).to(be_a(client.models.AnalyzerConfig))
        expect(analyzer.config.protocol_counters).not_to(be_empty)
        expect(analyzer.config.stream_counters).not_to(be_empty)
        return True, ['is valid packet analyzer']


class _be_valid_packet_analyzer_result(Matcher):
    def _match(self, result):
        expect(result).to(be_a(client.models.AnalyzerResult))
        expect(result.id).not_to(be_empty)
        expect(result.analyzer_id).not_to(be_empty)
        expect(result.active).not_to(be_none)
        expect(result.protocol_counters).to(be_a(client.models.AnalyzerProtocolCounters))
        expect(result.stream_counters).to(be_a(client.models.AnalyzerStreamCounters))
        expect(result.stream_counters.frame_count).not_to(be_none)
        return True, ['is valid packet analyzer result']


be_valid_packet_analyzer = _be_valid_packet_analyzer()
be_valid_packet_analyzer_result = _be_valid_packet_analyzer_result()
