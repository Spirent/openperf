from expects import expect, be_a, be_empty, be_none
from expects.matchers import Matcher

import client.models

class be_in_list(Matcher):
    def __init__(self, expected):
        self._expected = expected

    def _match(self, request):
        expect(request).not_to(be_empty)
        if request in self._expected:
            return True, ['Value in list']
        return False, ['Value not in list']


class _be_valid_dynamic_results(Matcher):
    def _match(self, result):
        expect(result).to(be_a(client.models.DynamicResults))
        return True, ['is valid Dynamic Results set']


class _be_valid_threshold(Matcher):
    def _match(self, result):
        expect(result).to(be_a(client.models.ThresholdResult))
        expect(result.id).not_to(be_empty)
        expect(result.value).to(be_a(float))
        expect(result.function).to(be_in_list(['dx', 'dxdy', 'dxdt']))
        expect(result.condition).to(be_in_list(['greater', 'greater_or_equal', 'less', 'less_or_equal', 'equal']))
        expect(result.stat_x).not_to(be_empty)
        expect(result.condition_true).to(be_a(int))
        expect(result.condition_false).to(be_a(int))
        return True, ['is valid Threshold']


class _be_valid_centroid(Matcher):
    def _match(self, request):
        expect(result).to(be_a(client.models.TDigestCentroid))
        expect(result.mean).to(be_a(float))
        expect(result.weight).to(be_a(int))
        return True, ['is valid T-Digest Centroid']


class _be_valid_tdigest(Matcher):
    def _match(self, result):
        expect(result).to(be_a(client.models.TDigestResult))
        expect(result.id).not_to(be_empty)
        expect(result.function).to(be_in_list({'dx', 'dxdy', 'dxdt'}))
        expect(result.stat_x).not_to(be_empty)
        return True, ['is valid T-Digest']


be_valid_tdigest = _be_valid_tdigest()
be_valid_threshold = _be_valid_threshold()
be_valid_centroid = _be_valid_centroid()
be_valid_dynamic_results = _be_valid_dynamic_results()
