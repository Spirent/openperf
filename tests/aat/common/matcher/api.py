from client.rest import ApiException
from expects.matchers import Matcher
from expects import expect, equal, have_keys, have_key

class has_location(Matcher):
    def __init__(self, expected):
        self._expected = expected

    def _match(self, subject):
        expect(subject).to(have_key('Location'))
        return subject['Location'] == self._expected, []

class raise_api_exception(Matcher):
    def __init__(self, expected, **kwargs):
        self._expected = expected
        self._otherExpected = kwargs

    def __repr__(self):
        return 'raise ApiException with HTTP status {0}'.format(self._expected)

    def _match(self, subject):
        try:
            subject()
        except ApiException as exc:
            expect(exc.status).to(equal(self._expected))

            if 'headers' in self._otherExpected:
                expect(exc.headers).to(have_keys(self._otherExpected['headers']))

            return True, [
                'returned HTTP status: {0} {1}'.format(exc.status, exc.reason),
                'Headers: {0}'.format(exc.headers),
                'Body: {0}'.format(exc.body),
            ]
        else:
            return False, ['no ApiException raised']
