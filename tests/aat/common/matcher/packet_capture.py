from expects import expect, be_a, be_empty, be_none
from expects.matchers import Matcher

import client.models


class _be_valid_packet_capture(Matcher):
    def _match(self, capture):
        expect(capture).to(be_a(client.models.PacketCapture))
        expect(capture.id).not_to(be_empty)
        expect(capture.source_id).not_to(be_empty)
        expect(capture.active).not_to(be_none)
        expect(capture.config).to(be_a(client.models.PacketCaptureConfig))
        return True, ['is valid packet capture']


class _be_valid_packet_capture_result(Matcher):
    def _match(self, result):
        expect(result).to(be_a(client.models.PacketCaptureResult))
        expect(result.id).not_to(be_empty)
        expect(result.capture_id).not_to(be_empty)
        expect(result.active).not_to(be_none)
        expect(result.state).not_to(be_none)
        return True, ['is valid packet capture result']


be_valid_packet_capture = _be_valid_packet_capture()
be_valid_packet_capture_result = _be_valid_packet_capture_result()
