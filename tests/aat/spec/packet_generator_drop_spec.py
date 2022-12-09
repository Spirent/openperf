from mamba import description, before, after
from expects import *
import os
import time

import client.api
import client.models
from common.helper import (check_modules_exists,
                           packet_generator_model)
from common.matcher import (be_valid_packet_generator,
                            be_valid_packet_generator_result)
from common import Config, Service


CONFIG = Config(os.path.join(os.path.dirname(__file__),
                             os.environ.get('MAMBA_CONFIG', 'config.yaml')))

POLL_INTERVAL = 0.025


def wait_until_done(gen_client, result_id, initial_sleep = None):
    if (initial_sleep):
        time.sleep(initial_sleep)

    while True:
        result = gen_client.get_packet_generator_result(result_id)
        expect(result).to(be_valid_packet_generator_result)
        if result.flow_counters.packets_actual > 0 and not result.active:
            break;
        time.sleep(POLL_INTERVAL)


with description('Packet Generator Drop Mode,', 'packet_generator_drop') as self:
    with description('Drops and Stops'):

        with before.all:
            service = Service(CONFIG.service('packet-generator-drop'))
            self.process = service.start()
            self.api = client.api.PacketGeneratorsApi(service.client())
            if not check_modules_exists(service.client(), 'packet-generator'):
                self.skip()

        with description('overload transmit path,'):
            with it('stops on-time and drops packets'):
                time = client.models.TrafficDurationTime()
                time.value = 100;
                time.units = "milliseconds"
                duration = client.models.TrafficDuration()
                duration.time = time
                gen = packet_generator_model(self.api.api_client)
                gen.config.duration = duration

                # Set a load the test environment can't possibly hit
                gen.config.load.burst_size = 64
                gen.config.load.rate.value = 148809523

                gen = self.api.create_packet_generator(gen)
                expect(gen).to(be_valid_packet_generator)

                result = self.api.start_packet_generator(gen.id)
                expect(result).to(be_valid_packet_generator_result)

                # Wait for automatic stop
                wait_until_done(self.api, result.id)

                result = self.api.get_packet_generator_result(result.id)
                expect(result).to(be_valid_packet_generator_result)

                expect(result.flow_counters.packets_actual).to(
                    be_below(result.flow_counters.packets_intended))
                expect(result.flow_counters.packets_dropped).to(be_above(0))
                expect(result.flow_counters.octets_dropped).to(be_above(0))

        with after.each:
            try:
                for gen in self.api.list_packet_generators():
                    if gen.active:
                        self.api.stop_packet_generator(gen.id)
                self.api.delete_packet_generators()

            except AttributeError:
                pass

        with after.all:
            try:
                self.process.terminate()
                self.process.wait()
            except AttributeError:
                pass
