from mamba import description, before, after
from expects import *
import os

import time

import client.api
import client.models
from common import Config, Service
from common.helper import (get_first_port_id,
                           get_second_port_id,
                           packet_generator_model,
                           packet_generator_config,
                           wait_for_learning_resolved)
from common.matcher import (be_valid_packet_analyzer,
                            be_valid_packet_generator,
                            raise_api_exception)
from common.helper import check_modules_exists
from common.helper import ipv4_interface

GENERATOR_CONFIG_NO_SOURCES = {
    'duration': {'continuous': True},
    'load': {'rate': 10},
    'protocol_counters': ['ethernet', 'ip', 'transport'],
    'traffic': [
        {
            'length': {'fixed': 128},
            'packet': [
                'ethernet',
                {'ipv4': {'destination': '198.18.15.20'}},
                'udp'
            ]
        }
    ]
}

def create_analyzer_with_filter(ana_api_client, id, source_id, filter):
    config = client.models.PacketAnalyzerConfig()
    config.filter = filter
    ta = client.models.PacketAnalyzer()
    ta.source_id = source_id
    ta.config = config
    ta.id = id
    ana = ana_api_client.create_packet_analyzer(ta)
    expect(ana).to(be_valid_packet_analyzer)

    return ana

CONFIG = Config(os.path.join(os.path.dirname(__file__),
                             os.environ.get('MAMBA_CONFIG', 'config.yaml')))

with description('MAC Learning,', 'learning') as self:
    with description('REST API'):

        with before.all:
            service = Service(CONFIG.service())
            self.process = service.start()
            self.ana_api = client.api.PacketAnalyzersApi(service.client())
            self.gen_api = client.api.PacketGeneratorsApi(service.client())
            self.intf_api = client.api.InterfacesApi(service.client())
            if not check_modules_exists(service.client(), 'packet-generator', 'packet-analyzer'):
                self.skip()

        with description('packet-generator, '):
            with description('IPv4, '):
                with before.each:
                        self.source_ip = "192.168.22.10"
                        self.source_mac = "aa:bb:cc:dd:ee:01"
                        self.target_ip = "192.168.22.1"
                        self.intf1 = ipv4_interface(self.intf_api.api_client, method="static", ipv4_address=self.source_ip, gateway=self.target_ip, mac_address=self.source_mac)
                        self.intf1.target_id = get_first_port_id(self.intf_api.api_client)
                        self.intf1.id = "interface-one"
                        self.intf_api.create_interface(self.intf1)

                        self.target_mac = "aa:bb:cc:dd:ee:20"
                        self.intf2 = ipv4_interface(self.intf_api.api_client, method="static", ipv4_address=self.target_ip, mac_address=self.target_mac)
                        self.intf2.port_id = get_second_port_id(self.intf_api.api_client)
                        self.intf2.id = "interface-two"
                        self.intf_api.create_interface(self.intf2)

                with description('with source addresses, '):
                    with before.each:
                        ana_filter = "ether dst " + self.target_mac
                        ana_filter += " and not ether src " + self.source_mac
                        ana_filter += " and not src host " + self.source_ip
                        self.filtered_analyzer = create_analyzer_with_filter(self.ana_api,
                                                                            'analyzer-one',
                                                                            get_second_port_id(self.ana_api.api_client),
                                                                            ana_filter)

                    with it('does not automatically populate from interface'):
                        gen = packet_generator_model(self.gen_api.api_client)
                        gen.target_id = "interface-one"
                        gen.id = "generator-one"
                        result = self.gen_api.create_packet_generator(gen)
                        expect(result).to(be_valid_packet_generator)

                        # Wait for learning to resolve.
                        expect(wait_for_learning_resolved(self.gen_api, "generator-one")).to(be_true)

                        # Start stuff
                        self.ana_api.start_packet_analyzer('analyzer-one')
                        self.gen_api.start_packet_generator('generator-one')

                        # Wait a sec for packets to transmit
                        time.sleep(0.5)

                        # Verify analyzer got packets that matched filter parameters
                        ana_results = self.ana_api.list_packet_analyzer_results(analyzer_id='analyzer-one')
                        expect(ana_results).to_not(be_empty)
                        expect(ana_results[0].flow_counters.frame_count).to(be_above(0))

                with description('without source addresses, '):
                    with before.each:
                        ana_filter = "src host " + self.source_ip
                        ana_filter += " and ether dst " + self.target_mac
                        ana_filter += " and ether src " + self.source_mac
                        self.filtered_analyzer = create_analyzer_with_filter(self.ana_api,
                                                                            'analyzer-one',
                                                                            get_second_port_id(self.ana_api.api_client),
                                                                            ana_filter)

                    with it('does automatically populate from interface'):
                        gen_cfg = packet_generator_config(**GENERATOR_CONFIG_NO_SOURCES)
                        gen = client.models.PacketGenerator()
                        gen.target_id = 'interface-one'
                        gen.id = 'generator-one'
                        gen.config = gen_cfg

                        gen_result = self.gen_api.create_packet_generator(gen)
                        expect(gen_result).to(be_valid_packet_generator)

                        # Wait for learning to resolve.
                        expect(wait_for_learning_resolved(self.gen_api, "generator-one")).to(be_true)

                        # Start stuff
                        self.ana_api.start_packet_analyzer('analyzer-one')
                        self.gen_api.start_packet_generator('generator-one')

                        # Wait a sec for packets to transmit
                        time.sleep(0.5)

                        # Verify analyzer got packets that matched filter parameters
                        ana_results = self.ana_api.list_packet_analyzer_results(analyzer_id='analyzer-one')
                        expect(ana_results).to_not(be_empty)
                        expect(ana_results[0].flow_counters.frame_count).to(be_above(0))


        with after.each:
            try:
                for gen in self.gen_api.list_packet_generators():
                    if gen.active:
                        self.gen_api.stop_packet_generator(gen.id)
                self.gen_api.delete_packet_generators()

                for ana in self.ana_api.list_packet_analyzers():
                    if ana.active:
                        self.ana_api.stop_packet_analyzer(ana.id)
                self.ana_api.delete_packet_analyzers()

                for intf in self.intf_api.list_interfaces():
                    self.intf_api.delete_interface(intf.id)
            except AttributeError:
                pass
            self.generator = None
            self.result = None

        with after.all:
            try:
                self.process.terminate()
                self.process.wait()
            except AttributeError:
                pass
