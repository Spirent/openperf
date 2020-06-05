from expects import expect, be_a, be_empty, be_none, be_above, be_above_or_equal, equal
from expects.matchers import Matcher

import client.models
import datetime


class _be_valid_packet_analyzer(Matcher):
    def _match(self, analyzer):
        expect(analyzer).to(be_a(client.models.PacketAnalyzer))
        expect(analyzer.id).not_to(be_empty)
        expect(analyzer.source_id).not_to(be_empty)
        expect(analyzer.active).not_to(be_none)
        expect(analyzer.config).to(be_a(client.models.PacketAnalyzerConfig))
        expect(analyzer.config.flow_counters).not_to(be_empty)
        expect(analyzer.config.protocol_counters).not_to(be_empty)
        return True, ['is valid packet analyzer']


class _be_valid_packet_analyzer_result(Matcher):
    def _match(self, result):
        expect(result).to(be_a(client.models.PacketAnalyzerResult))
        expect(result.id).not_to(be_empty)
        expect(result.analyzer_id).not_to(be_empty)
        expect(result.active).not_to(be_none)
        expect(result.flow_counters).to(be_valid_packet_analyzer_flow_counter)
        expect(result.protocol_counters).to(be_a(client.models.PacketAnalyzerProtocolCounters))
        return True, ['is valid packet analyzer result']


class _be_valid_non_zero_summary_statistic(Matcher):
    def _match(self, stats):
        expect(stats.summary).not_to(be_none)
        expect(stats.units).not_to(be_none)
        expect(stats.summary.max).to(be_above_or_equal(stats.summary.min))

        # Work around jitter_ipdv stat, which can be negative
        if stats.summary.min > 0:
            expect(stats.summary.total).to(be_above(0))
            expect(stats.summary.max).to(be_above(0))
            if stats.summary.min != stats.summary.max:
                expect(stats.summary.std_dev).to(be_above(0))

        return True, ['is valid non zero summary statistic']


class _be_valid_packet_analyzer_flow_counter(Matcher):
    def _match(self, counters):
        expect(counters).to(be_a(client.models.PacketAnalyzerFlowCounters))
        expect(counters.frame_count).not_to(be_none)
        # Check optional summary statistics
        if counters.frame_count > 0:
            if counters.frame_length:
                expect(counters.frame_length).to(be_valid_non_zero_summary_statistic)
            if counters.latency:
                expect(counters.latency).to(be_valid_non_zero_summary_statistic)
            if counters.sequence:
                total = (counters.sequence.dropped
                         + counters.sequence.duplicate
                         + counters.sequence.late
                         + counters.sequence.reordered
                         + counters.sequence.in_order)
                expect(total).to(equal(counters.frame_count))

        if counters.frame_count > 1:
            if counters.interarrival:
                expect(counters.interarrival).to(be_valid_non_zero_summary_statistic)
            if counters.jitter_ipdv:
                expect(counters.jitter_ipdv).to(be_valid_non_zero_summary_statistic)
            if counters.jitter_rfc:
                expect(counters.jitter_rfc).to(be_valid_non_zero_summary_statistic)

            # 0 < timestamp_first <= timestamp_last
            expect(counters.timestamp_last).to(be_above(counters.timestamp_first))
            duration = counters.timestamp_last - counters.timestamp_first
            expect(duration).to(be_above_or_equal(datetime.timedelta(0)))

        return True, ['is valid packet analyzer flow counter']


class _be_valid_receive_flow(Matcher):
    def _match(self, flow):
        expect(flow.id).not_to(be_none)
        expect(flow.analyzer_result_id).not_to(be_none)
        expect(flow.counters).to(be_valid_packet_analyzer_flow_counter)
        return True, ['is valid receive flow']


be_valid_packet_analyzer = _be_valid_packet_analyzer()
be_valid_packet_analyzer_result = _be_valid_packet_analyzer_result()
be_valid_packet_analyzer_flow_counter = _be_valid_packet_analyzer_flow_counter()
be_valid_non_zero_summary_statistic = _be_valid_non_zero_summary_statistic()
be_valid_receive_flow = _be_valid_receive_flow()
