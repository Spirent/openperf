#!/usr/bin/env python3

import argparse
import collections
import datetime
import json
import ipaddress
import pprint
import random
import re
import requests
import time


###
# Pistache appears to have a limit on the size of JSON data included with
# HTTP requests.  These values specify the number of items to include in
# the bulk-create/bulk-delete requests and are experimentally determined,
# e.g. they worked for me.
###
ADD_PAGE_SIZE = 12
ID_PAGE_SIZE = 32


def get_random_mac(port_id):
    octets = list()
    octets.append(random.randint(0, 255) & 0xfc)
    octets.append((port_id >> 16) & 0xff)
    octets.append(port_id & 0xff)
    for _i in range(3):
        octets.append(random.randint(0, 255))

    return '{0:02x}:{1:02x}:{2:02x}:{3:02x}:{4:02x}:{5:02x}'.format(*octets)


def distribute(total, buckets, n):
    base = int(total / buckets)
    return base + 1 if n < total % buckets else base


# XXX: Only IPv4 is supported right now
def get_static_interface_config(tag, port_id, network, index):
    eth_protocol = {'eth': { 'mac_address': get_random_mac(0) } }
    if network.version == 4:
        ip_protocol = {
            'ipv4': {
                'method': 'static',
                'static': {
                    'address': str(network[index]),
                    'prefix_length': int(network.prefixlen),
                    'gateway': str(network[-1])
                }
            }
        }

    return {
        'port_id': str(port_id),
        'id': '{}-inf-{}'.format(tag, index),
        'config': {
            'protocols': [ eth_protocol, ip_protocol ],
        }
    }

def get_generator_config(idx, src_if, dst_macs, dst_ips, args):
    assert idx > 0, 'Cannot use idx of 0'

    eth_hdr = {
        'ethernet': {},
        'modifiers': {
            'tie': 'zip',
            'items': [
                {
                    'name': 'destination',
                    'mac': {'list': dst_macs},
                    'permute': False
                }
            ]
        }
    }

    ipv4_hdr = {
        'ipv4': {},
        'modifiers': {
            'tie': 'zip',
            'items': [
                {
                    'name': 'destination',
                    'ipv4': {'list': dst_ips},
                    'permute': False
                }
            ]
        }
    }

    udp_hdr = {'udp': {}}

    definition = {'packet': {'protocols': [eth_hdr, ipv4_hdr, udp_hdr]},
                  'length': {'fixed': 1024},
                  'signature': {'stream_id': idx,
                                'latency': 'end_of_frame'}}

    load = {'burst_size': 1,
            'rate': {'value': distribute(args.rate, args.count, idx),
                     'period': 'seconds'},
            'units': 'frames'}

    config = {'duration': {'continuous': True},
              'load': load,
              'traffic': [definition]}

    return {'id': '{}-gen-{}'.format(args.tag, idx),
            'target_id': src_if,
            'config': config}



def create_interfaces(url, tag, port_id, network, count, offset):
    items = list()
    for idx in range(1 + offset, 1 + offset + count):
        items.append(get_static_interface_config(tag, port_id, network, idx))

    total = 0
    while total < count:
        r = requests.post("/".join([url, 'interfaces/x/bulk-create']),
                          json={'items': items[total:total + ADD_PAGE_SIZE]})
        r.raise_for_status()
        total += ADD_PAGE_SIZE


def get_port_interface_map(args):
    get = requests.get("/".join([args.url, 'interfaces']))
    get.raise_for_status()
    m = collections.defaultdict(list)
    for inf, port in [(inf['id'], inf['port_id']) for inf in filter(
            lambda inf: inf['id'].startswith(args.tag), get.json())]:
        m[port].append(inf)

    return m


def get_interface_addrs(args, inf):
    get = requests.get("/".join([args.url, 'interfaces', inf]))
    get.raise_for_status()
    protocols = get.json()['config']['protocols']
    return (protocols[0]['eth']['mac_address'],
            protocols[1]['ipv4']['static']['address'])


def create_generators(args):
    configs = list()
    m = get_port_interface_map(args)
    idx = 0
    for src_port, src_infs in m.items():
        for dst_port, dst_infs in m.items():
            if src_port == dst_port:
                continue
            for s in src_infs:
                idx += 1
                dst_macs, dst_ips = zip(*[get_interface_addrs(args, inf) for inf in dst_infs])
                configs.append(get_generator_config(idx, s, dst_macs, dst_ips, args))

    assert len(configs) > 0, 'No generator configs!'

    total = 0
    while total < len(configs):
        r = requests.post("/".join([args.url, 'packet/generators/x/bulk-create']),
                          json={'items': configs[total:total + ADD_PAGE_SIZE]})
        r.raise_for_status()
        total += ADD_PAGE_SIZE


def create_interfaces_and_generators(args):
    offset = args.offset
    for idx, port in enumerate(args.ports):
        nb_ifs = distribute(args.count, len(args.ports), idx)
        if not nb_ifs:
            continue
        create_interfaces(args.url, args.tag, port, args.network, nb_ifs, offset)
        offset += nb_ifs

    create_generators(args)


def get_interface_ids(args):
    # Retrieve the list of interface ids from the openperf instance
    get = requests.get("/".join([args.url, 'interfaces']))
    get.raise_for_status()
    return [inf['id'] for inf in filter(lambda inf: inf['id'].startswith(args.tag), get.json())]


def delete_interfaces(args):
    ids = get_interface_ids(args)
    total = 0
    count = len(ids)
    while total < count:
        # Unlike with additions, it is NOT ok to go past the end of the list
        result = requests.post("/".join([args.url, 'interfaces/x/bulk-delete']),
                                json={'ids': ids[total:min(total + ID_PAGE_SIZE, count)]})
        result.raise_for_status()
        total += ID_PAGE_SIZE


def get_generator_ids(args):
    # Retrieve the list of generator ids from the openperf instance
    get = requests.get("/".join([args.url, 'packet/generators']))
    get.raise_for_status()
    return [gen['id'] for gen in filter(lambda gen: gen['id'].startswith(args.tag), get.json())]


def do_bulk_generator_request(args, op):
    ids = get_generator_ids(args)
    print(ids)
    total = 0
    count = len(ids)
    while total < count:
        result = requests.post("/".join([args.url, 'packet/generators/x', op]),
                               json={'ids': ids[total:min(total + ID_PAGE_SIZE, count)]})
        result.raise_for_status()
        total += ID_PAGE_SIZE


def delete_generators(args):
    do_bulk_generator_request(args, 'bulk-delete')


def delete_interfaces_and_generators(args):
    delete_generators(args)
    delete_interfaces(args)


def get_unresolved_generators(args):
    get = requests.get("/".join([args.url, 'packet/generators']))
    get.raise_for_status()
    return [gen['id'] for gen in filter(lambda gen: gen['learning'] != 'resolved', get.json())]


def start_generators(args):
    # Verify that all learning is 'resolved'
    abort = time.time() + 5
    unresolved = get_unresolved_generators(args)
    while len(unresolved) and time.time() < abort:
        unresolved = get_unresolved_generators(args)
        time.sleep(1)

    assert len(unresolved) == 0, 'Generators have unresolved addresses!'

    do_bulk_generator_request(args, 'bulk-start')


def get_result_ids(args):
    # Retrieve the list of generator ids from the openperf instance
    get = requests.get("/".join([args.url, 'packet/generator-results']))
    get.raise_for_status()
    return [result['id'] for result in filter(
        lambda result: result['generator_id'].startswith(args.tag), get.json())]


def iso8601_to_datetime(s):
    # Python's datetime can't handle nanoseconds, so strip them off
    return datetime.datetime.strptime(s[:-4], '%Y-%m-%dT%H:%M:%S.%f')


def aggregate_results(args):
    octets = 0
    packets = 0
    start = None
    stop = None

    for handle in get_result_ids(args):
        r = requests.get("/".join([args.url, 'packet/generator-results/', handle]))
        counters = r.json()['flow_counters']
        octets += counters['octets_actual']
        packets += counters['packets_actual']
        local_start = iso8601_to_datetime(counters['timestamp_first'])
        local_stop = iso8601_to_datetime(counters['timestamp_last'])
        start = local_start if not start else min(start, local_start)
        stop = local_stop if not stop else max(stop, local_stop)

    if start and stop:
        delta = (stop - start).total_seconds()
        print('{} packets ({} octets) in {} seconds ({:.2f} pps)'.format(
            packets, octets, delta, packets / delta if delta else 0))
    else:
        print('No results!')


def stop_generators(args):
    do_bulk_generator_request(args, 'bulk-stop')
    aggregate_results(args)


def main():
    parser = argparse.ArgumentParser(description="Bulk learning testing for openperf")
    parser.add_argument('url')
    parser.add_argument('action', choices=['create', 'delete', 'start', 'stop'])
    parser.add_argument('--tag', default='learning', type=str,
                        help='prefix for interfaces and generators')
    parser.add_argument('--ports', nargs='+', type=str,
                        help='ports to use for interfaces and generators (add only)')
    parser.add_argument('--count', default=2, type=int,
                        help='number of interfaces and generators to create (add only)')
    parser.add_argument('--network', default='198.18.0.0/15', type=ipaddress.IPv4Network,
                        help='specify the interface network (add only)')
    parser.add_argument('--offset', default=0, type=int,
                        help='specify the index of the first network host address (add only)')
    parser.add_argument('--rate', default=100, type=int,
                        help='specify the aggregate packet rate in packets/sec (add only)')
    args=parser.parse_args()

    action_map = {
        'create': create_interfaces_and_generators,
        'delete': delete_interfaces_and_generators,
        'start': start_generators,
        'stop': stop_generators
    }

    action_map[args.action](args)


if __name__ == "__main__":
    main()
