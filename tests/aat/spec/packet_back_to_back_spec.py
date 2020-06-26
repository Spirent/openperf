import client.api
import datetime
import os
import time

from common import Config, Service
from common.helper import make_generator_config
from common.matcher import (be_valid_packet_analyzer,
                            be_valid_packet_analyzer_result,
                            be_valid_packet_generator,
                            be_valid_packet_generator_result,
                            be_valid_receive_flow,
                            be_valid_transmit_flow)
from expects import *


###
# Spec constants
###
CONFIG = Config(os.path.join(os.path.dirname(__file__),
                             os.environ.get('MAMBA_CONFIG', 'config.yaml')))
POLL_INTERVAL = 0.025
TIME_FUDGE = datetime.timedelta(milliseconds=5)

###
# Packet configurations
#
# XXX: Theoretically, one should be able to use any packet definition here,
# however, the AAT environment uses ring devices, which require parsing
# packets using software. And unfortunately, the DPDK packet parsing function
# doesn't parse MPLS packets correctly (as of v19.08).
#
# Caveat utilitor!
###
ETH_IPV4_UDP = [
    {'ethernet': {'source': '10:94:00:00:aa:bb',
                  'destination': '10:94:00:00:bb:cc'}},
    {'ipv4': {'source': '198.18.0.1',
              'destination': '198.18.0.2'}},
    'udp'
]

ETH_VLAN_IPV4_UDP = [
    {'ethernet': {'source': '10:94:00:00:aa:bb',
                  'destination': '10:94:00:00:bb:cc'}},
    {'vlan': {'id': 100}},
    {'ipv4': {'source': '198.18.0.1',
              'destination': '198.18.0.2'}},
    'udp'
]

ETH_IPV6_TCP = [
    {'ethernet': {'source': '10:94:00:00:aa:bb',
                  'destination': '10:94:00:00:bb:cc'}},
    {'ipv6': {'source': '2001:db8::1',
              'destination': '2001:db8::2'}},
    'tcp'
]

ETH_IPV4_UDP_MODIFIERS_ZIP = [
    {'ethernet': {'source': '10:94:00:00:aa:bb',
                  'destination': '10:94:00:00:bb:cc'}},
    {'ipv4': {
        'modifiers': {
            'items': [
                {'destination': {
                    'sequence': {'count': 10,
                                 'start': '198.18.200.1'}}
                },
                {'source': {
                    'list': ['198.18.100.1',
                             '198.18.100.2',
                             '198.18.100.3',
                             '198.18.100.5',
                             '198.18.100.8',
                             '198.18.100.13',
                             '198.18.100.21',
                             '198.18.100.34',
                             '198.18.100.55',
                             '198.18.100.89']
                }}
            ],
            'tie': 'zip'}
        },
    },
    {'udp': {
        'modifiers': {
            'items': [
                {'destination': {
                    'sequence': {'count': 20,
                                 'start': 3357 }
                }}
            ]
        }
    }}
]

ETH_IPV6_UDP_MODIFIERS_PRODUCT = [
    {'ethernet': {'source': '10:94:00:00:aa:bb',
                  'destination': '10:94:00:00:bb:cc'}},
    {'ipv6': {
        'modifiers': {
            'items': [
                {'destination': {
                    'sequence': {'count': 10,
                                 'start': '2001:db8::200:1'}}
                },
                {'source': {
                    'list': ['2001:db8::100:1',
                             '2001:db8::100:2',
                             '2001:db8::100:3',
                             '2001:db8::100:5',
                             '2001:db8::100:8',
                             '2001:db8::100:13',
                             '2001:db8::100:21',
                             '2001:db8::100:34',
                             '2001:db8::100:55',
                             '2001:db8::100:89']
                }}
            ],
            'tie': 'cartesian'}
        },
    },
    {'udp': {
        'modifiers': {
            'items': [
                {'destination': {
                    'sequence': {'count': 4,
                                 'start': 3357 }
                }}
            ]
        }
    }}
]

CUSTOM_DATA = "TG9yZW0gaXBzdW0gZG9sb3Igc2l0IGFtZXQsIGNvbnNlY3RldHVyIGFkaXBpc2NpbmcgZWxpdCwg\
c2VkIGRvIGVpdXNtb2QgdGVtcG9yIGluY2lkaWR1bnQgdXQgbGFib3JlIGV0IGRvbG9yZSBtYWdu\
YSBhbGlxdWEuIFV0IGVuaW0gYWQgbWluaW0gdmVuaWFtLCBxdWlzIG5vc3RydWQgZXhlcmNpdGF0\
aW9uIHVsbGFtY28gbGFib3JpcyBuaXNpIHV0IGFsaXF1aXAgZXggZWEgY29tbW9kbyBjb25zZXF1\
YXQuIER1aXMgYXV0ZSBpcnVyZSBkb2xvciBpbiByZXByZWhlbmRlcml0IGluIHZvbHVwdGF0ZSB2\
ZWxpdCBlc3NlIGNpbGx1bSBkb2xvcmUgZXUgZnVnaWF0IG51bGxhIHBhcmlhdHVyLiBFeGNlcHRl\
dXIgc2ludCBvY2NhZWNhdCBjdXBpZGF0YXQgbm9uIHByb2lkZW50LCBzdW50IGluIGN1bHBhIHF1\
aSBvZmZpY2lhIGRlc2VydW50IG1vbGxpdCBhbmltIGlkIGVzdCBsYWJvcnVtLgo="

CUSTOM_L2_PACKET = [
    {'custom': {'data': CUSTOM_DATA,
                'layer': 'ethernet'}}
]

CUSTOM_PROTOCOL_WITH_MODIFIERS = [
    {'ethernet': {'source': '10:94:00:00:aa:bb',
                  'destination': '10:94:00:00:bb:cc'}},
    {'ipv4': {'source': '198.18.15.10',
              'destination': '198.18.15.20'}},
    {'custom': {'data': CUSTOM_DATA,
                'layer': 'protocol',
                'modifiers': {
                    'items': [
                        {'data': {
                            'offset': 0,
                            'sequence': {'start': 101,
                                         'count': 10}
                        }},
                        {'data': {
                            'offset': 12,
                            'list': ['2001:db8::100:1',
                                     '2001:db8::100:2',
                                     '2001:db8::100:3',
                                     '2001:db8::100:5',
                                     '2001:db8::100:8']
                        }}
                    ],
                    'tie': 'cartesian'
                }}
    }
]

###
# Analyzer configurations
###
ANALYZER_CONFIG_NO_SIGS = {
    'protocol': ['ethernet', 'ip', 'transport'],
    'flow': ['frame_count', 'frame_length', 'interarrival_time']
}

ANALYZER_CONFIG_SIGS = {
    'protocol': ['ethernet', 'ip', 'transport'],
    'flow': ['frame_count', 'frame_length', 'interarrival_time',
             'jitter_ipdv', 'jitter_rfc', 'latency', 'advanced_sequencing']
}

###
# Generator configurations
###
GENERATOR_CONFIG_NO_SIGS = {
    'duration': {'frames': 100},
    'load': {'rate': 1000},
    'protocol_counters': ['ethernet', 'ip', 'transport'],
    'traffic': [
        {
            'length': {'fixed': 128},
            'packet': ETH_VLAN_IPV4_UDP,
        }
    ],
}

GENERATOR_CONFIG_SIGS = {
    'duration': {'frames': 1000 },
    'load': {'burst_size': 4,
             'rate': 10000 },
    'protocol_counters': ['ethernet', 'ip', 'transport'],
    'traffic': [
        {
            'length': {'list': [64, 128, 256, 512, 1024, 1280, 1518]},
            'packet': ETH_IPV4_UDP,
            'signature': {'stream_id': 1,
                          'latency': 'end_of_frame'}
        }
    ]
}

GENERATOR_CONFIG_CUSTOM_PACKET = {
    'duration': {'frames': 100},
    'load': {'rate': 1000},
    'protocol_counters': ['ethernet', 'ip', 'transport'],
    'traffic': [
        {
            'length': {'fixed': 128},
            'packet': CUSTOM_L2_PACKET,
        }
    ],
}

GENERATOR_CONFIG_MULTI_DEFS = {
    'duration': {'frames': 100},
    'load': {'rate': 1000},
    'protocol_counters': ['ethernet', 'ip', 'transport'],
    'traffic': [
        {
            'length': {'fixed': 128},
            'packet': ETH_VLAN_IPV4_UDP,
            'weight': 4
        },
        {
            'length': {'fixed': 1280},
            'packet': ETH_IPV6_TCP,
            'weight': 1
        }
    ],
}

GENERATOR_CONFIG_ZIP_MODS = {
    'duration': {'frames': 200},
    'load': {'burst_size': 16,
             'rate': 10000},
    'traffic': [
        {
            'length': {'fixed': 128},
            'packet': ETH_IPV4_UDP_MODIFIERS_ZIP,
        }
    ],
}

GENERATOR_CONFIG_PRODUCT_MODS = {
    'duration': {'frames': 200},
    'load': {'burst_size': 32,
             'rate': 10000},
    'traffic': [
        {
            'length': {'fixed': 128},
            'packet': ETH_IPV6_UDP_MODIFIERS_PRODUCT,
        }
    ],
}

GENERATOR_CONFIG_PxP_MODS = {
    'duration': {'frames': 500},
    'load': {'burst_size': 64,
             'rate': 10000},
    'traffic': [
        {
            'length': {'fixed': 128},
            'packet': ETH_IPV6_UDP_MODIFIERS_PRODUCT,
            'tie': 'cartesian'
        }
    ],
}

GENERATOR_CONFIG_CUSTOM_MODIFIERS = {
    'duration': {'frames': 100},
    'load': {'rate': 1000},
    'traffic': [
        {
            'length': {'fixed': 128},
            'packet': CUSTOM_PROTOCOL_WITH_MODIFIERS,
            'signature': {'stream_id': 1,
                          'latency': 'end_of_frame'}
        }
    ],
}

# Not sure if the CI system can actually handle this, but we
# want the reset/toggle tests to generate traffic as fast as
# possible.
FAST_LOAD = {
    'burst_size': 32,
    'rate': 1000000
}

GENERATOR_CONFIG_CONTINUOUS_A = {
    'duration': {'continuous': True},
    'load': FAST_LOAD,
    'traffic': [
        {
            'length': {'fixed': 128},
            'packet': ETH_IPV4_UDP,
            'signature': {'stream_id': 1,
                          'latency': 'end_of_frame'}
        }
    ],
}

GENERATOR_CONFIG_CONTINUOUS_B = {
    'duration': {'continuous': True},
    'load': FAST_LOAD,
    'traffic': [
        {
            'length': {'fixed': 128},
            'packet': ETH_IPV4_UDP,
            'signature': {'stream_id': 1,
                          'latency': 'end_of_frame'},
            'weight': 1
        },
        {
            'length': {'fixed': 512},
            'packet': ETH_IPV6_TCP,
            'signature': {'stream_id': 2,
                          'latency': 'end_of_frame'},
            'weight': 1
        }
    ],
}

###
# Utility functions
###
def get_ports(api_client):
    ports_api = client.api.PortsApi(api_client)
    ports = ports_api.list_ports()
    expect(len(ports)).to(be_above_or_equal(2))
    return ports


def get_analyzer_port_id(api_client):
    return get_ports(api_client)[0].id


def get_generator_port_id(api_client):
    return get_ports(api_client)[1].id


def get_analyzer_model(api_client, analyzer_config):
    config = client.models.PacketAnalyzerConfig()
    if 'protocol' in analyzer_config:
        config.protocol_counters = analyzer_config['protocol']
    if 'flow' in analyzer_config:
        config.flow_counters = analyzer_config['flow']

    analyzer = client.models.PacketAnalyzer()
    analyzer.source_id = get_analyzer_port_id(api_client)
    analyzer.config = config

    return analyzer


def get_generator_model(api_client, generator_config):
    config = make_generator_config(**generator_config)

    generator = client.models.PacketGenerator()
    generator.target_id = get_generator_port_id(api_client)
    generator.config = config

    return generator


def wait_until_done(gen_client, result_id, initial_sleep = None):
    if (initial_sleep):
        time.sleep(initial_sleep)

    while True:
        result = gen_client.get_packet_generator_result(result_id)
        expect(result).to(be_valid_packet_generator_result)
        if not result.active:
            break;
        time.sleep(POLL_INTERVAL)


def wait_until_more_generator_results(gen_client, result_id, tx_threshold = None):
    min_tx = tx_threshold if tx_threshold else 0
    while True:
        time.sleep(POLL_INTERVAL)
        result = gen_client.get_packet_generator_result(result_id)
        if result.flow_counters.packets_actual > min_tx:
            break


def configure_and_run_test(api_client, ana_config, gen_config):
    """Build the specified config; start it, return the final results"""
    ana_api = client.api.PacketAnalyzersApi(api_client)
    gen_api = client.api.PacketGeneratorsApi(api_client)

    # Create analyzer/generator objects
    analyzer = ana_api.create_packet_analyzer(get_analyzer_model(api_client, ana_config))
    expect(analyzer).to(be_valid_packet_analyzer)

    generator = gen_api.create_packet_generator(get_generator_model(api_client, gen_config))
    expect(generator).to(be_valid_packet_generator)

    # Start both objects and wait until done...
    ana_result = ana_api.start_packet_analyzer(analyzer.id)
    expect(ana_result).to(be_valid_packet_analyzer_result)
    expect(ana_result.active).to(be_true)
    expect(ana_result.analyzer_id).to(equal(analyzer.id))

    gen_result = gen_api.start_packet_generator(generator.id)
    expect(gen_result).to(be_valid_packet_generator_result)
    expect(gen_result.active).to(be_true)
    expect(gen_result.generator_id).to(equal(generator.id))

    wait_until_done(gen_api, gen_result.id)
    ana_api.stop_packet_analyzer(analyzer.id)

    # Retrieve and return final results
    ana_result = ana_api.get_packet_analyzer_result(ana_result.id)
    expect(ana_result).to(be_valid_packet_analyzer_result)
    expect(ana_result.active).to(be_false)

    gen_result = gen_api.get_packet_generator_result(gen_result.id)
    expect(gen_result).to(be_valid_packet_generator_result)
    expect(gen_result.active).to(be_false)
    expect(gen_result.remaining).to(be_none)

    return ana_result, gen_result


def validate_durations(ana_result, gen_result):
    rx_duration = ana_result.flow_counters.timestamp_last - ana_result.flow_counters.timestamp_first
    expect(rx_duration).to(be_above(datetime.timedelta(0)))

    tx_duration = gen_result.flow_counters.timestamp_last - gen_result.flow_counters.timestamp_first
    expect(tx_duration).to(be_above(datetime.timedelta(0)))

    # Compare tx/rx durations
    expect(rx_duration).to(be_within(tx_duration - TIME_FUDGE, tx_duration + TIME_FUDGE))


def get_min_frame_length(gen_config):
    min_length = 65536
    for definition in gen_config['traffic']:
        len_config = definition['length']
        if 'fixed' in len_config:
            min_length = min(min_length, len_config['fixed'])
        elif 'list' in len_config:
            for l in len_config['list']:
                min_length = min(min_length, l)
        else:
            assert False, 'case not handled'
    return min_length


def get_max_frame_length(gen_config):
    max_length = 0
    for definition in gen_config['traffic']:
        len_config = definition['length']
        if 'fixed' in len_config:
            max_length = max(max_length, len_config['fixed'])
        elif 'list' in len_config:
            for l in len_config['list']:
                max_length = max(max_length, l)
        else:
            assert False, 'case not handled'
    return max_length


def validate_frame_length(ana_result, gen_config):
    expect(ana_result.flow_counters.frame_length).not_to(be_none)
    min_frame_length = get_min_frame_length(gen_config)
    max_frame_length = get_max_frame_length(gen_config)
    expect(ana_result.flow_counters.frame_length.summary.min).to(equal(min_frame_length))
    expect(ana_result.flow_counters.frame_length.summary.max).to(equal(max_frame_length))
    if min_frame_length != max_frame_length:
        expect(ana_result.flow_counters.frame_length.summary.std_dev).not_to(equal(0))


def validate_rx_flows(api_client, ana_result):
    ana_api = client.api.PacketAnalyzersApi(api_client)

    for flow_id in ana_result.flows:
        flow = ana_api.get_rx_flow(flow_id)
        expect(flow).to(be_valid_receive_flow)

    expect([f.id for f in ana_api.list_rx_flows() if f.id in ana_result.flows]).not_to(be_empty)


def validate_tx_flows(api_client, gen_result):
    gen_api = client.api.PacketGeneratorsApi(api_client)

    for flow_id in gen_result.flows:
        flow = gen_api.get_tx_flow(flow_id)
        expect(flow).to(be_valid_transmit_flow)

    expect([f.id for f in gen_api.list_tx_flows() if f.id in gen_result.flows]).not_to(be_empty)


###
# Begin test proper
###
with description('Packet back to back', 'packet_b2b') as self:
    with description('generation and analysis,'):

        with before.all:
            service = Service(CONFIG.service())
            self.process = service.start()
            self.client = service.client()
            self.analyzer_api = client.api.PacketAnalyzersApi(service.client())
            self.generator_api = client.api.PacketGeneratorsApi(service.client())

        with description('with single traffic definition,'):
            with description('without signatures,'):
                with it('succeeds'):
                    ana_result, gen_result = configure_and_run_test(self.client,
                                                                    ANALYZER_CONFIG_NO_SIGS,
                                                                    GENERATOR_CONFIG_NO_SIGS)

                    # Validate results
                    exp_flow_count = 1
                    expect(len(ana_result.flows)).to(equal(exp_flow_count))
                    expect(len(gen_result.flows)).to(equal(exp_flow_count))

                    # Check analyzer protocol counters
                    exp_frame_count = GENERATOR_CONFIG_NO_SIGS['duration']['frames']
                    expect(ana_result.protocol_counters.ethernet.ip).to(equal(0))
                    expect(ana_result.protocol_counters.ethernet.vlan).to(equal(exp_frame_count))
                    expect(ana_result.protocol_counters.ip.ipv4).to(equal(exp_frame_count))
                    expect(ana_result.protocol_counters.ip.ipv6).to(equal(0))
                    expect(ana_result.protocol_counters.transport.udp).to(equal(exp_frame_count))
                    expect(ana_result.protocol_counters.transport.tcp).to(equal(0))

                    # Check analyzer flow counters
                    expect(ana_result.flow_counters.frame_count).to(equal(exp_frame_count))
                    expect(ana_result.flow_counters.frame_length).not_to(be_none)
                    expect(ana_result.flow_counters.interarrival).not_to(be_none)
                    expect(ana_result.flow_counters.jitter_ipdv).to(be_none)
                    expect(ana_result.flow_counters.jitter_rfc).to(be_none)
                    expect(ana_result.flow_counters.latency).to(be_none)
                    expect(ana_result.flow_counters.sequence).to(be_none)

                    # Check generator flow counters
                    expect(gen_result.flow_counters.packets_actual).to(equal(exp_frame_count))

                    # Check generator protocol counters
                    expect(gen_result.protocol_counters.ethernet.ip).to(equal(0))
                    expect(gen_result.protocol_counters.ethernet.vlan).to(equal(exp_frame_count))
                    expect(gen_result.protocol_counters.ip.ipv4).to(equal(exp_frame_count))
                    expect(gen_result.protocol_counters.ip.ipv6).to(equal(0))
                    expect(gen_result.protocol_counters.transport.udp).to(equal(exp_frame_count))
                    expect(gen_result.protocol_counters.transport.tcp).to(equal(0))

                    # Check miscellaneous results
                    validate_durations(ana_result, gen_result)
                    validate_frame_length(ana_result, GENERATOR_CONFIG_NO_SIGS)
                    validate_rx_flows(self.client, ana_result)
                    validate_tx_flows(self.client, gen_result)

            with description('with signatures,'):
                with it('succeeds'):
                    ana_result, gen_result = configure_and_run_test(self.client,
                                                                    ANALYZER_CONFIG_SIGS,
                                                                    GENERATOR_CONFIG_SIGS)

                    # Validate results
                    exp_flow_count = 1
                    expect(len(ana_result.flows)).to(equal(exp_flow_count))
                    expect(len(gen_result.flows)).to(equal(exp_flow_count))

                    # Check analyzer protocol counters
                    exp_frame_count = GENERATOR_CONFIG_SIGS['duration']['frames']
                    expect(ana_result.protocol_counters.ethernet.ip).to(equal(exp_frame_count))
                    expect(ana_result.protocol_counters.ethernet.vlan).to(equal(0))
                    expect(ana_result.protocol_counters.ip.ipv4).to(equal(exp_frame_count))
                    expect(ana_result.protocol_counters.ip.ipv6).to(equal(0))
                    expect(ana_result.protocol_counters.transport.udp).to(equal(exp_frame_count))
                    expect(ana_result.protocol_counters.transport.tcp).to(equal(0))

                    # Check analyzer flow counters; all stats should be present
                    expect(ana_result.flow_counters.frame_count).to(equal(exp_frame_count))
                    expect(ana_result.flow_counters.interarrival).not_to(be_none)
                    expect(ana_result.flow_counters.jitter_ipdv).not_to(be_none)
                    expect(ana_result.flow_counters.jitter_rfc).not_to(be_none)
                    expect(ana_result.flow_counters.latency).not_to(be_none)
                    expect(ana_result.flow_counters.sequence).not_to(be_none)

                    # Check generator flow counters
                    expect(gen_result.flow_counters.packets_actual).to(equal(exp_frame_count))

                    # Check generator protocol counters
                    expect(gen_result.protocol_counters.ethernet.ip).to(equal(exp_frame_count))
                    expect(gen_result.protocol_counters.ethernet.vlan).to(equal(0))
                    expect(gen_result.protocol_counters.ip.ipv4).to(equal(exp_frame_count))
                    expect(gen_result.protocol_counters.ip.ipv6).to(equal(0))
                    expect(gen_result.protocol_counters.transport.udp).to(equal(exp_frame_count))
                    expect(gen_result.protocol_counters.transport.tcp).to(equal(0))

                    # Check miscellaneous results
                    validate_durations(ana_result, gen_result)
                    validate_frame_length(ana_result, GENERATOR_CONFIG_SIGS)
                    validate_rx_flows(self.client, ana_result)
                    validate_tx_flows(self.client, gen_result)

            with description('with custom packet,'):
                with it('succeeds'):
                    ana_result, gen_result = configure_and_run_test(self.client,
                                                                    ANALYZER_CONFIG_NO_SIGS,
                                                                    GENERATOR_CONFIG_CUSTOM_PACKET)
                    # Validate results
                    exp_flow_count = 1
                    expect(len(ana_result.flows)).to(equal(exp_flow_count))
                    expect(len(gen_result.flows)).to(equal(exp_flow_count))

                    # Check analyzer protocol counters
                    exp_frame_count = GENERATOR_CONFIG_CUSTOM_PACKET['duration']['frames']
                    expect(ana_result.flow_counters.frame_count).to(equal(exp_frame_count))
                    expect(ana_result.protocol_counters.ethernet.arp).to(equal(0))
                    expect(ana_result.protocol_counters.ethernet.fcoe).to(equal(0))
                    # XXX: Software decoder default to ip for unrecognized traffic
                    # expect(ana_result.protocol_counters.ethernet.ip).to(equal(0))
                    expect(ana_result.protocol_counters.ethernet.lldp).to(equal(0))
                    expect(ana_result.protocol_counters.ethernet.pppoe).to(equal(0))
                    expect(ana_result.protocol_counters.ethernet.qinq).to(equal(0))
                    expect(ana_result.protocol_counters.ethernet.vlan).to(equal(0))
                    expect(ana_result.protocol_counters.ethernet.mpls).to(equal(0))
                    expect(ana_result.protocol_counters.ip.ipv4).to(equal(0))
                    expect(ana_result.protocol_counters.ip.ipv6).to(equal(0))
                    expect(ana_result.protocol_counters.transport.udp).to(equal(0))
                    expect(ana_result.protocol_counters.transport.tcp).to(equal(0))

                    # Check generator flow counters
                    expect(gen_result.flow_counters.packets_actual).to(equal(exp_frame_count))

                    # Check generator protocol counters
                    expect(gen_result.protocol_counters.ethernet.arp).to(equal(0))
                    expect(gen_result.protocol_counters.ethernet.fcoe).to(equal(0))
                    expect(gen_result.protocol_counters.ethernet.ip).to(equal(0))
                    expect(gen_result.protocol_counters.ethernet.lldp).to(equal(0))
                    expect(gen_result.protocol_counters.ethernet.pppoe).to(equal(0))
                    expect(gen_result.protocol_counters.ethernet.qinq).to(equal(0))
                    expect(gen_result.protocol_counters.ethernet.vlan).to(equal(0))
                    expect(gen_result.protocol_counters.ethernet.mpls).to(equal(0))
                    expect(gen_result.protocol_counters.ip.ipv4).to(equal(0))
                    expect(gen_result.protocol_counters.ip.ipv6).to(equal(0))
                    expect(gen_result.protocol_counters.transport.udp).to(equal(0))
                    expect(gen_result.protocol_counters.transport.tcp).to(equal(0))

                    # Check miscellaneous results
                    validate_durations(ana_result, gen_result)
                    validate_frame_length(ana_result, GENERATOR_CONFIG_CUSTOM_PACKET)
                    validate_rx_flows(self.client, ana_result)
                    validate_tx_flows(self.client, gen_result)

        with description('with multiple traffic definitions,'):
            with it('succeeds'):
                ana_result, gen_result = configure_and_run_test(self.client,
                                                                ANALYZER_CONFIG_NO_SIGS,
                                                                GENERATOR_CONFIG_MULTI_DEFS)

                # Validate results
                exp_flow_count = 2
                expect(len(ana_result.flows)).to(equal(exp_flow_count))
                expect(len(gen_result.flows)).to(equal(exp_flow_count))

                # Check analyzer protocol counters
                exp_frame_count = GENERATOR_CONFIG_MULTI_DEFS['duration']['frames']
                exp_flow1_count = exp_frame_count * 4 / 5
                exp_flow2_count = exp_frame_count / 5
                expect(ana_result.protocol_counters.ethernet.ip).to(equal(exp_flow2_count))
                expect(ana_result.protocol_counters.ethernet.vlan).to(equal(exp_flow1_count))
                expect(ana_result.protocol_counters.ip.ipv4).to(equal(exp_flow1_count))
                expect(ana_result.protocol_counters.ip.ipv6).to(equal(exp_flow2_count))
                expect(ana_result.protocol_counters.transport.udp).to(equal(exp_flow1_count))
                expect(ana_result.protocol_counters.transport.tcp).to(equal(exp_flow2_count))

                # Check analyzer flow counters
                expect(ana_result.flow_counters.frame_count).to(equal(exp_frame_count))

                # Check generator flow counters
                expect(gen_result.flow_counters.packets_actual).to(equal(exp_frame_count))

                # Check generator protocol counters
                expect(gen_result.protocol_counters.ethernet.ip).to(equal(exp_flow2_count))
                expect(gen_result.protocol_counters.ethernet.vlan).to(equal(exp_flow1_count))
                expect(gen_result.protocol_counters.ip.ipv4).to(equal(exp_flow1_count))
                expect(gen_result.protocol_counters.ip.ipv6).to(equal(exp_flow2_count))
                expect(gen_result.protocol_counters.transport.udp).to(equal(exp_flow1_count))
                expect(gen_result.protocol_counters.transport.tcp).to(equal(exp_flow2_count))

                # Check miscellaneous results
                validate_durations(ana_result, gen_result)
                validate_frame_length(ana_result, GENERATOR_CONFIG_MULTI_DEFS)
                validate_rx_flows(self.client, ana_result)
                validate_tx_flows(self.client, gen_result)

        # Note: For the modifier tests, we're really only checking the flow/frame counts
        # to make sure they are correct.
        with description('with modifiers,'):
            with description('zip modifiers,'):
                with it('succeeds'):
                    ana_result, gen_result = configure_and_run_test(self.client,
                                                                    ANALYZER_CONFIG_NO_SIGS,
                                                                    GENERATOR_CONFIG_ZIP_MODS)

                    # Validate flow counts
                    exp_flow_count = 20
                    expect(len(ana_result.flows)).to(equal(exp_flow_count))
                    expect(len(gen_result.flows)).to(equal(exp_flow_count))

                    # Validate frame count
                    exp_frame_count = GENERATOR_CONFIG_ZIP_MODS['duration']['frames']
                    expect(ana_result.flow_counters.frame_count).to(equal(exp_frame_count))
                    expect(gen_result.flow_counters.packets_actual).to(equal(exp_frame_count))
                    validate_rx_flows(self.client, ana_result)
                    validate_tx_flows(self.client, gen_result)

            with description('cartesian modifiers,'):
                with description('in protocol,'):
                    with it('succeeds'):
                        ana_result, gen_result = configure_and_run_test(self.client,
                                                                        ANALYZER_CONFIG_NO_SIGS,
                                                                        GENERATOR_CONFIG_PRODUCT_MODS)

                        # Validate flow counts
                        exp_flow_count = 100
                        expect(len(ana_result.flows)).to(equal(exp_flow_count))
                        expect(len(gen_result.flows)).to(equal(exp_flow_count))

                        # Validate frame count
                        exp_frame_count = GENERATOR_CONFIG_PRODUCT_MODS['duration']['frames']
                        expect(ana_result.flow_counters.frame_count).to(equal(exp_frame_count))
                        expect(gen_result.flow_counters.packets_actual).to(equal(exp_frame_count))
                        validate_rx_flows(self.client, ana_result)
                        validate_tx_flows(self.client, gen_result)

                with description('in custom protocol,'):
                    with it('succeeds'):
                        # XXX: Need sigs for analyzer to recognize custom protocol flows
                        ana_result, gen_result = configure_and_run_test(self.client,
                                                                        ANALYZER_CONFIG_SIGS,
                                                                        GENERATOR_CONFIG_CUSTOM_MODIFIERS)
                        #print(ana_result)
                        # Validate results
                        # XXX: analyzer doesn't recognize this protocol, so it can't differentiate flows!
                        exp_flow_count = 50
                        expect(len(ana_result.flows)).to(equal(exp_flow_count))
                        expect(len(gen_result.flows)).to(equal(exp_flow_count))

                        # Validate frame count
                        exp_frame_count = GENERATOR_CONFIG_CUSTOM_MODIFIERS['duration']['frames']
                        expect(ana_result.flow_counters.frame_count).to(equal(exp_frame_count))
                        expect(gen_result.flow_counters.packets_actual).to(equal(exp_frame_count))
                        validate_rx_flows(self.client, ana_result)
                        validate_tx_flows(self.client, gen_result)

                with description('in protocol and packet,'):
                    with it('succeeds'):
                        ana_result, gen_result = configure_and_run_test(self.client,
                                                                        ANALYZER_CONFIG_NO_SIGS,
                                                                        GENERATOR_CONFIG_PxP_MODS)

                        # Validate flow counts
                        exp_flow_count = 400
                        expect(len(ana_result.flows)).to(equal(exp_flow_count))
                        expect(len(gen_result.flows)).to(equal(exp_flow_count))

                        # Validate frame count
                        exp_frame_count = GENERATOR_CONFIG_PxP_MODS['duration']['frames']
                        expect(ana_result.flow_counters.frame_count).to(equal(exp_frame_count))
                        expect(gen_result.flow_counters.packets_actual).to(equal(exp_frame_count))
                        validate_rx_flows(self.client, ana_result)
                        validate_tx_flows(self.client, gen_result)

            with description('analyzer reset,'):
                with it('succeeds'):
                    ana_api = client.api.PacketAnalyzersApi(self.client)
                    gen_api = client.api.PacketGeneratorsApi(self.client)

                    # Create analyzer/generator objects
                    analyzer = ana_api.create_packet_analyzer(get_analyzer_model(self.client,
                                                                                 ANALYZER_CONFIG_SIGS))
                    expect(analyzer).to(be_valid_packet_analyzer)

                    generator = gen_api.create_packet_generator(get_generator_model(self.client,
                                                                                    GENERATOR_CONFIG_CONTINUOUS_A))
                    expect(generator).to(be_valid_packet_generator)

                    # Start both objects
                    ana_result1 = ana_api.start_packet_analyzer(analyzer.id)
                    expect(ana_result1).to(be_valid_packet_analyzer_result)
                    expect(ana_result1.active).to(be_true)
                    expect(ana_result1.analyzer_id).to(equal(analyzer.id))

                    gen_result = gen_api.start_packet_generator(generator.id)
                    expect(gen_result).to(be_valid_packet_generator_result)
                    expect(gen_result.active).to(be_true)
                    expect(gen_result.generator_id).to(equal(generator.id))

                    # Wait until we transmit some packets
                    wait_until_more_generator_results(gen_api, gen_result.id)

                    # Reset the analyzer results
                    ana_result2 = ana_api.reset_packet_analyzer(analyzer.id)

                    # Now wait until the generator transmit some more
                    gen_curr = gen_api.get_packet_generator_result(gen_result.id)
                    wait_until_more_generator_results(gen_api, gen_result.id, gen_curr.flow_counters.packets_actual)

                    gen_api.stop_packet_generator(generator.id)
                    ana_api.stop_packet_analyzer(analyzer.id)

                    # Retrieve final results
                    ana_result1 = ana_api.get_packet_analyzer_result(ana_result1.id)
                    expect(ana_result1).to(be_valid_packet_analyzer_result)
                    expect(ana_result1.active).to(be_false)
                    ana_result2 = ana_api.get_packet_analyzer_result(ana_result2.id)
                    expect(ana_result2).to(be_valid_packet_analyzer_result)
                    expect(ana_result2.active).to(be_false)

                    gen_result = gen_api.get_packet_generator_result(gen_result.id)
                    expect(gen_result).to(be_valid_packet_generator_result)
                    expect(gen_result.active).to(be_false)
                    expect(gen_result.remaining).to(be_none)

                    # Sanity check results; we want the total rx
                    # packet count from both results to match the
                    # transmit count.
                    tx_pkt_count = gen_result.flow_counters.packets_actual
                    rx_pkt_count = ana_result1.flow_counters.frame_count + ana_result2.flow_counters.frame_count
                    expect(tx_pkt_count).to(equal(rx_pkt_count))

            with description('generator toggle,'):
                with it('succeeds'):
                    ana_api = client.api.PacketAnalyzersApi(self.client)
                    gen_api = client.api.PacketGeneratorsApi(self.client)

                    # Create analyzer/generator objects
                    analyzer = ana_api.create_packet_analyzer(get_analyzer_model(self.client,
                                                                                 ANALYZER_CONFIG_SIGS))
                    expect(analyzer).to(be_valid_packet_analyzer)

                    generator1 = gen_api.create_packet_generator(get_generator_model(self.client,
                                                                                     GENERATOR_CONFIG_CONTINUOUS_A))
                    expect(generator1).to(be_valid_packet_generator)

                    # Start both objects
                    ana_result = ana_api.start_packet_analyzer(analyzer.id)
                    expect(ana_result).to(be_valid_packet_analyzer_result)
                    expect(ana_result.active).to(be_true)
                    expect(ana_result.analyzer_id).to(equal(analyzer.id))

                    gen_result1 = gen_api.start_packet_generator(generator1.id)
                    expect(gen_result1).to(be_valid_packet_generator_result)
                    expect(gen_result1.active).to(be_true)
                    expect(gen_result1.generator_id).to(equal(generator1.id))

                    # Wait until we transmit some packets
                    wait_until_more_generator_results(gen_api, gen_result1.id)

                    # Create a new generator and switch to it
                    generator2 = gen_api.create_packet_generator(get_generator_model(self.client,
                                                                                     GENERATOR_CONFIG_CONTINUOUS_B))
                    expect(generator2).to(be_valid_packet_generator)

                    toggle = client.models.TogglePacketGeneratorsRequest()
                    toggle.replace = generator1.id
                    toggle._with = generator2.id

                    gen_result2 = gen_api.toggle_packet_generators(toggle)
                    expect(gen_result2).to(be_valid_packet_generator_result)
                    expect(gen_result2.active).to(be_true)
                    expect(gen_result2.generator_id).to(equal(generator2.id))

                    # Now wait for generator2 to transmit some packets
                    wait_until_more_generator_results(gen_api, gen_result2.id)

                    # Finally, stop objects and check results
                    gen_api.stop_packet_generator(generator2.id)
                    ana_api.stop_packet_analyzer(analyzer.id)

                    # Retrieve final results
                    ana_result = ana_api.get_packet_analyzer_result(ana_result.id)
                    expect(ana_result).to(be_valid_packet_analyzer_result)
                    expect(ana_result.active).to(be_false)

                    gen_result1 = gen_api.get_packet_generator_result(gen_result1.id)
                    expect(gen_result1).to(be_valid_packet_generator_result)
                    expect(gen_result1.active).to(be_false)
                    gen_result2 = gen_api.get_packet_generator_result(gen_result2.id)
                    expect(gen_result2).to(be_valid_packet_generator_result)
                    expect(gen_result2.active).to(be_false)

                    # Check results.  The total tx/rx counts should be
                    # the same.
                    tx_pkt_count = gen_result1.flow_counters.packets_actual + gen_result2.flow_counters.packets_actual
                    rx_pkt_count = ana_result.flow_counters.frame_count
                    expect(tx_pkt_count).to(equal(rx_pkt_count))

                    # Additionally, the analyzer should show
                    # 2 streams with no dropped packets in the
                    # sequence stats.
                    expect(len(ana_result.flows)).to(equal(2))
                    expect(ana_result.flow_counters.frame_count).to(equal(ana_result.flow_counters.sequence.in_order))

        with after.each:
            try:
                for gen in self.generator_api.list_packet_generators():
                    if gen.active:
                        self.generator_api.stop_packet_generator(gen.id)
                self.generator_api.delete_packet_generators()
            except AttributeError:
                pass

            try:
                for ana in self.analyzer_api.list_packet_analyzers():
                    if ana.active:
                        self.analyzer_api.stop_packet_analyzer(ana.id)
                self.analyzer_api.delete_packet_analyzers()
            except AttributeError:
                pass

        with after.all:
            try:
                self.process.terminate()
                self.process.wait()
            except AttributeError:
                pass
