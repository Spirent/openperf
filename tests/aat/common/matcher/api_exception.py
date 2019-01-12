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

class raise_api_exception_extended(Matcher):
    def __init__(self, expected):
        self._expected = expected

    def __repr__(self):
        return 'raise ApiException containing at least HTTP values {0}'.format(self._expected)

    def _match(self, subject):
        try:
            subject()
        except ApiException as exc:
            result = True
            errors = ""

            #assume we always want to check status code.
            if self._expected['status'] != exc.status:
                errors += "Value for HTTP status code '%d' not as expected. " % exc.status
                result &= False
            else:
                result &= True
            if 'headers' in self._expected:
                #Be careful. Headers are expressed as strings. It's fragile but matches exact API output better.
                for header, value in self._expected['headers'].items():
                    if header in exc.headers:
                        if value == exc.headers[header]:
                            result &= True
                        else:
                            errors += "Value '%s' for header '%s' not as expected. " % (exc.headers[header], header)
                            result &= False
                    else:
                        errors += "Expected header '%s' missing. " % header
                        result &= False
            #TODO: implement support for checking HTTP body element.

            return result, [
                'returned HTTP status: {0} {1}'.format(exc.status, exc.reason),
                'Headers: {0}'.format(exc.headers),
                'Body: {0}'.format(exc.body),
                'Errors: {0}'.format(errors),
             ]
        else:
            return False, ['no ApiException raised']
