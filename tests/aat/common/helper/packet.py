from expects import *
import time
from common.helper.traffic import (make_traffic_definition,
                                   make_traffic_duration,
                                   make_traffic_load,
                                   make_traffic_template)

import client.api
import client.models


SEQ_MODIFIER_PACKET = [
    {'ethernet': {'source': '10:94:00:00:aa:bb',
                  'modifiers': {
                      'items': [
                          {'destination': {
                              'sequence': {'count': 10,
                                           'start': '00:00:01:00:00:01'}}
                          }
                      ]
                  }}
    },
    {'ipv4': {
        'modifiers': {
            'items': [
                {'source': {
                    'sequence': {'count': 10,
                                 'start': '198.18.15.1'}}
                },
                {'destination': {
                    'sequence': {'count': 10,
                                 'start': '198.18.16.1'}}
                }
            ],
            'tie': 'zip'}
    }},
    'udp'
]


LIST_MODIFIER_PACKET = [
    {'ethernet': {'source': '10:94:00:00:aa:bb',
                  'destination': '10:94:00:00:bb:cc'}},
    {'ipv4': {
        'modifiers': {
            'items': [
                {'source': {
                    'list': ['198.18.15.1',
                             '198.18.15.3',
                             '198.18.15.5']
                }},
                {'destination': {
                    'list': ['198.18.16.1',
                             '198.18.16.3',
                             '198.18.16.5']
                }}
            ],
            'tie': 'zip'}
    }},
    'udp'
]


GENERATOR_CONFIG_DEFAULT = {
    'duration': {'continuous': True},
    'load': {'rate': 10},
    'protocol_counters': ['ethernet', 'ip', 'transport'],
    'traffic': [
        {
            'length': {'fixed': 128},
            'packet': [
                {'ethernet': {'source': '10:94:00:00:aa:bb',
                              'destination': '10:94:00:00:bb:cc'}},
                {'ipv4': {'source': '198.18.15.10',
                          'destination': '198.18.15.20'}},
                'udp'
            ]
        }
    ]
}


def get_first_port_id(api_client):
    ports_api = client.api.PortsApi(api_client)
    ports = ports_api.list_ports()
    expect(ports).not_to(be_empty)
    return ports[0].id

def get_second_port_id(api_client):
    ports_api = client.api.PortsApi(api_client)
    ports = ports_api.list_ports()
    expect(ports).not_to(be_empty)
    return ports[1].id

def default_traffic_packet_template_with_seq_modifiers(permute_flag=None):
    model = make_traffic_template(SEQ_MODIFIER_PACKET)

    for protocol in model.protocols:
        if protocol.modifiers:
            for modifier in protocol.modifiers.items:
                modifier.permute = permute_flag if permute_flag else False

    return model


def default_traffic_packet_template_with_list_modifiers(permute_flag=None):
    model = make_traffic_template(LIST_MODIFIER_PACKET)

    for protocol in model.protocols:
        if protocol.modifiers:
            for modifier in protocol.modifiers.items:
                modifier.permute = permute_flag if permute_flag else False

    return model


def packet_generator_config(**kwargs):
    duration_config = kwargs['duration'] if 'duration' in kwargs else DURATION_CONFIG
    load_config = kwargs['load'] if 'load' in kwargs else LOAD_CONFIG
    packet_config = kwargs['traffic'] if 'traffic' in kwargs else TRAFFIC_CONFIG

    config = client.models.PacketGeneratorConfig()
    config.duration = make_traffic_duration(duration_config)
    config.load = make_traffic_load(load_config)
    config.traffic = list(map(make_traffic_definition, packet_config))
    config.order = kwargs['order'] if 'order' in kwargs else 'round-robin'
    config.protocol_counters = kwargs['protocol_counters'] if 'protocol_counters' in kwargs else None

    return config


def config_model():
    return packet_generator_config(**GENERATOR_CONFIG_DEFAULT)


def packet_generator_model(api_client):
    config = packet_generator_config(**GENERATOR_CONFIG_DEFAULT)

    gen = client.models.PacketGenerator()
    gen.target_id = get_first_port_id(api_client)
    gen.config = config

    return gen


def packet_generator_models(api_client):
    ports_api = client.api.PortsApi(api_client)
    ports = ports_api.list_ports()
    expect(ports).to(have_len(be_above(1)))

    gen1 = packet_generator_model(api_client)
    gen1.target_id = ports[0].id
    gen2 = packet_generator_model(api_client)
    gen2.target_id = ports[1].id

    return [gen1, gen2]

def wait_for_learning_resolved(api_client, generator_id, **kwargs):
    poll_interval = kwargs['poll_interval'] if 'poll_interval' in kwargs else 1
    max_poll_count = kwargs['max_poll_count'] if 'max_poll_count' in kwargs else 3

    for _ in range(max_poll_count):
        gen = api_client.get_packet_generator(generator_id)

        if gen.learning == 'resolved':
            return True

        time.sleep(poll_interval)

    return False