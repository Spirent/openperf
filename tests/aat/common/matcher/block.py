from expects import expect, be_a, be_empty, be_none, be_below_or_equal
from expects.matchers import Matcher

import client.models


class _be_valid_block_device(Matcher):
    def _match(self, device):
        expect(device).to(be_a(client.models.BlockDevice))
        expect(device.id).not_to(be_empty)
        expect(device.path).not_to(be_empty)
        expect(device.size).not_to(be_none)
        expect(device.usable).not_to(be_none)
        return True, ['is valid block device']

class _be_valid_block_file(Matcher):
    def _match(self, file):
        expect(file).to(be_a(client.models.BlockFile))
        expect(file.id).not_to(be_empty)
        expect(file.path).not_to(be_empty)
        expect(file.size).not_to(be_none)
        expect(file.init_percent_complete).not_to(be_none)
        expect(file.state).not_to(be_empty)
        return True, ['is valid block file']

class _be_valid_block_generator(Matcher):
    def _match(self, generator):
        expect(generator).to(be_a(client.models.BlockGenerator))
        expect(generator.id).not_to(be_empty)
        expect(generator.resource_id).not_to(be_empty)
        expect(generator.running).not_to(be_none)
        expect(generator.config).to(be_a(client.models.BlockGeneratorConfig))
        expect(generator.config.pattern).not_to(be_empty)
        return True, ['is valid block generator']


class _be_valid_block_generator_result(Matcher):
    def _match(self, result):
        expect(result).to(be_a(client.models.BlockGeneratorResult))
        expect(result.id).not_to(be_empty)
        expect(result.generator_id).not_to(be_empty)
        expect(result.active).not_to(be_none)
        expect(result.timestamp_first).not_to(be_none)
        expect(result.timestamp_last).not_to(be_none)
        expect(result.timestamp_first).to(be_below_or_equal(result.timestamp_last))
        expect(result.read).to(be_a(client.models.BlockGeneratorStats))
        expect(result.write).to(be_a(client.models.BlockGeneratorStats))
        return True, ['is valid block generator result']


be_valid_block_generator = _be_valid_block_generator()
be_valid_block_generator_result = _be_valid_block_generator_result()
be_valid_block_device = _be_valid_block_device()
be_valid_block_file = _be_valid_block_file()
