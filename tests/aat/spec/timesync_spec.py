from expects import expect, be_empty, equal
import os
import sys

import client.api
import client.models
from common import Config, Service
from common.matcher import (be_valid_counter,
                            be_valid_keeper,
                            be_valid_source,
                            raise_api_exception)


CONFIG = Config(os.path.join(os.path.dirname(__file__), os.environ.get('MAMBA_CONFIG', 'config.yaml')))

# localhost should always be resolvable, so it's safe to test the API
# with.  We don't necessarily expect it to be an actual time source
# though, so none of these tests check for clock syncrhonization
DUMMY_NTP_HOST = '127.0.0.1'


def get_ntp_timesource(hostname, id=None):
    ntp_config = client.models.TimeSourceConfigNtp()
    ntp_config.hostname = hostname

    # XXX: Our test container doesn't have an /etc/services file, so we
    # need to explicitly state the port.  The default port of "ntp"
    # will cause a lookup failure.
    ntp_config.port = "123"

    config = client.models.TimeSourceConfig()
    config.ntp = ntp_config

    s = client.models.TimeSource()
    s.kind = 'ntp'
    s.id = 'test-source-' + str(id)
    s.config = config

    return s


with description('Timesync,', 'timesync') as self:
    with before.all:
        service = Service(CONFIG.service())
        self.process = service.start()
        self.counters_api = client.api.TimeCountersApi(service.client())
        self.keeper_api = client.api.TimeKeeperApi(service.client())
        self.sources_api = client.api.TimeSourcesApi(service.client())

    with description('counters,'):
        with description('unsupported method,'):
            with it('returns 405'):
                expect(lambda: self.counters_api.api_client.call_api(
                    '/time-counters', 'PUT')).to(
                        raise_api_exception(405, headers={'Allow': "GET"}))

        with description('GET,'):
            with description('list,'):
                with it('succeeds'):
                    counters = self.counters_api.list_time_counters()
                    expect(counters).not_to(be_empty)
                    for counter in counters:
                        expect(counter).to(be_valid_counter)

            with description('valid counter,'):
                with it('succeeds'):
                    for counter in self.counters_api.list_time_counters():
                        expect(self.counters_api.get_time_counter(counter.id)).to(be_valid_counter)

            with description('non-existent counter,'):
                with it('returns 404'):
                    expect(lambda: self.counters_api.get_time_counter('foo')).to(raise_api_exception(404))

    with description('keeper,'):
        with description('unsupported method,'):
            with it('returns 405'):
                expect(lambda: self.counters_api.api_client.call_api(
                    '/time-keeper', 'PUT')).to(
                        raise_api_exception(405, headers={'Allow': "GET"}))

        with description('GET,'):
            with it('succeeds'):
                expect(self.keeper_api.get_time_keeper()).to(be_valid_keeper)

    with description('sources,'):
        with before.each:
            self.cleanup = list()

        with description('unsupported method,'):
            with it('returns 405'):
                expect(lambda: self.sources_api.api_client.call_api(
                    '/time-sources', 'PUT')).to(
                        raise_api_exception(405, headers={'Allow': 'GET, POST'}))

        with description('POST,'):
            with it('succeeds'):
                source = get_ntp_timesource(DUMMY_NTP_HOST, 'post')
                result = self.sources_api.create_time_source(source)
                expect(self.sources_api.get_time_source(source.id)).to(be_valid_source)
                self.cleanup.append(result)

        with description('GET,'):
            with before.each:
                self.cleanup.append(
                    self.sources_api.create_time_source(get_ntp_timesource(DUMMY_NTP_HOST, 'get'))
                )

            with description('list,'):
                with it('succeeds'):
                    sources = self.sources_api.list_time_sources()
                    expect(sources).not_to(be_empty)
                    for src in sources:
                        expect(src).to(be_valid_source)

            with description('valid source,'):
                with it('succeeds'):
                    for src in self.cleanup:
                        expect(self.sources_api.get_time_source(src.id)).to(be_valid_source)

            with description('non-existent source,'):
                with it('returns 404'):
                    expect(lambda: self.sources_api.get_time_source('foo')).to(raise_api_exception(404))

        with description('DELETE,'):
            with description('valid source,'):
                with it('succeeds'):
                    source = get_ntp_timesource(DUMMY_NTP_HOST, 'delete')
                    result = self.sources_api.create_time_source(source)
                    expect(self.sources_api.get_time_source(source.id)).to(be_valid_source)
                    self.sources_api.delete_time_source(source.id)

                    # Verify deletion
                    expect(lambda: self.sources_api.get_time_source(source.id)).to(raise_api_exception(404))

            with description('non-existent source,'):
                with it('succeeds'):
                    self.sources_api.delete_time_source('not-here')

        with description('behavior,'):
            with description('only one source allowed,'):
                ###
                # XXX: Likely to change
                # We currently can only have one source, so make sure that adding
                # a source deletes any previously added source
                ###
                with it('succeeds'):
                    source0 = get_ntp_timesource(DUMMY_NTP_HOST, 'source0')
                    self.sources_api.create_time_source(source0)
                    expect(self.sources_api.get_time_source(source0.id)).to(be_valid_source)

                    source1 = get_ntp_timesource(DUMMY_NTP_HOST, 'source1')
                    self.sources_api.create_time_source(source1)
                    expect(self.sources_api.get_time_source(source1.id)).to(be_valid_source)
                    expect(lambda: self.sources_api.get_time_source(source0.id)).to(raise_api_exception(404))

            with description('keeper acquires source id,'):
                with it('succeeds'):
                    source = get_ntp_timesource(DUMMY_NTP_HOST, 'check0')
                    self.sources_api.create_time_source(source)
                    expect(self.sources_api.get_time_source(source.id)).to(be_valid_source)
                    keeper = self.keeper_api.get_time_keeper()
                    expect(keeper).to(be_valid_keeper)
                    expect(keeper.time_source_id).to(equal(source.id))

        with after.each:
            for src in self.cleanup:
                try:
                    self.sources_api.delete_time_source(src)
                except:
                    pass

    with after.all:
        try:
            self.process.terminate()
            self.process.wait()
        except AttributeError:
            pass
