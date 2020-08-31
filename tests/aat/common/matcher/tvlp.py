from expects import expect, be_a, be_empty, be_none
from expects.matchers import Matcher

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

class _be_valid_tvlp_result(Matcher):
    def _match(self, config):
        expect(config).to(be_a(client.models.TvlpResult))
        expect(config.id).not_to(be_none)
        expect(config.tvlp_id).not_to(be_none)
        return True, ['is valid TVLP result']

be_valid_tvlp_configuration = _be_valid_tvlp_configuration()
be_valid_tvlp_result = _be_valid_tvlp_result()