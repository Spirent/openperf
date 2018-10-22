from client.rest import ApiException
from expects.matchers import Matcher


class raise_api_exception(Matcher):
    def __init__(self, expected):
        self._expected = expected

    def __repr__(self):
        return 'raise ApiException with HTTP status {0}'.format(self._expected)

    def _match(self, subject):
        try:
            subject()
        except ApiException as exc:
            return exc.status == self._expected, [
                'returned HTTP status: {0} {1}'.format(exc.status, exc.reason),
                'Headers: {0}'.format(exc.headers),
                'Body: {0}'.format(exc.body),
            ]
        else:
            return False, ['no ApiException raised']
