#!/usr/bin/env python3

import argparse
import datetime
import json
import ipaddress
import os
import pprint
import random
import requests


ADD_PAGE_SIZE=16
ID_PAGE_SIZE=128

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


def get_generator_blob(idx, port, args):
    src_macs = [get_random_mac(0) for x in range(0, args.flows)]

    eth_hdr = {
        'ethernet': {},
        'modifiers': {
            'tie': 'zip',
            'items': [
                {
                    'name': 'source',
                    'mac': {
                        'sequence': {
                            'count': args.flows,
                            'start': get_random_mac(0)
                        }
                    },
                    'permute': False
                },
                {
                    'name': 'destination',
                    'mac': {
                        'sequence': {
                            'count': args.flows,
                            'start': get_random_mac(1)
                        }
                    },
                    'permute': False
                }
            ]
        }
    }
    ip_hdr = {
        'ipv4': {},
        'modifiers': {
            'tie': 'zip',
            'items': [
                {
                    'name': 'source',
                    'ipv4': {
                        'sequence': {
                            'count': args.flows,
                            'start': str(args.src_network[args.src_offset + args.flows * idx])
                        }
                    },
                    'permute': False
                },
                {
                    'name': 'destination',
                    'ipv4': {
                        'sequence': {
                            'count': args.flows,
                            'start': str(args.dst_network[args.dst_offset + args.flows * idx])
                        }
                    },
                    'permute': False
                }
            ]
        }
    }
    udp_hdr = {'udp': {}}

    definition = {'packet': {'modifier_tie': 'zip',
                             'protocols': [eth_hdr, ip_hdr, udp_hdr]},
                             #'protocols': [eth_hdr, ip_hdr]},
                  'length': {'fixed': args.frame_length}}

    if args.signatures:
        definition['signature'] = {'stream_id': idx + 1,
                                   'latency': 'end_of_frame'}
        if args.fill:
            fill = {args.fill: True} if args.fill == 'prbs' else {args.fill: 0}
            definition['signature']['fill'] = fill

    load = {'burst_size': args.burst_size,
            'rate': {
                'value': distribute(args.rate, args.count, idx),
                'period': 'seconds'
            },
            'units': 'frames'}

    config = {
        'duration': {'continuous': True},
        'load': load,
        'traffic': [definition]
    }

    return {
        'id': args.tag + str(idx),
        'target_id': port,
        'config': config
    }


def create_generators(args):
    idx = 0
    configs = list()
    for i, port in enumerate(args.ports):
        count = distribute(args.count, len(args.ports), i)
        for j in range(0, count):
            configs.append(get_generator_blob(idx, port, args))
            idx += 1

    total = 0
    while total < args.count:
        r = requests.post("/".join([args.url, 'packet/generators/x/bulk-create']),
                          json={'items': configs[total:total + ADD_PAGE_SIZE]})
        r.raise_for_status()
        total += ADD_PAGE_SIZE


def get_generator_ids(args):
    # Retrieve the list of generator ids from the openperf instance
    get = requests.get("/".join([args.url, 'packet/generators']))
    get.raise_for_status()
    return [gen['id'] for gen in filter(lambda gen: gen['id'].startswith(args.tag), get.json())]


def do_bulk_id_request(args, op):
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
    do_bulk_id_request(args, 'bulk-delete')


def start_generators(args):
    do_bulk_id_request(args, 'bulk-start')


def stop_generators(args):
    do_bulk_id_request(args, 'bulk-stop')


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
        print('{} packets ({} octets) in {} seconds ({:.2f} pps, {:.2f} octets/sec)'.format(
            packets, octets, delta,
            packets / delta if delta else 0,
            octets / delta if delta else 0))
    else:
        print('No results!')


###
# I shamelessly stole this from stackoverflow because my local python3 version
# lacks argparse.BooleanOptionalAction. :(
# See https://stackoverflow.com/questions/15008758/parsing-boolean-values-with-argparse
###
def str_to_bool(v):
    if isinstance(v, bool):
       return v
    if v.lower() in ('yes', 'true', 't', 'y', '1'):
        return True
    elif v.lower() in ('no', 'false', 'f', 'n', '0'):
        return False
    else:
        raise argparse.ArgumentTypeError('Boolean value expected.')


def main():
    parser = argparse.ArgumentParser(description='Bulk generator testing for openperf')

    if 'URL' not in os.environ:
        parser.add_argument('url')
    else:
        parser.add_argument('--url', default=os.environ.get('URL'),
                            help='Dummy argument for environment variable')

    parser.add_argument('action', choices=['create', 'delete', 'start', 'stop', 'aggregate'])
    parser.add_argument('--tag', default='generator', type=str,
                        help='prefix for generator ids')
    parser.add_argument('--flows', default=1, type=int,
                        help='the number of flows per generator')
    parser.add_argument('--count', default=1, type=int,
                        help='the number of generators to create/delete')
    parser.add_argument('--rate', default=1000000, type=int,
                        help="aggregate rate for all generators (pps)")
    parser.add_argument('--burst-size', default=64, type=int,
                        help='generator burst size')
    parser.add_argument('--frame-length', default=128, type=int,
                        help="frame length (including CRC)")
    parser.add_argument('--signatures', default=False, type=str_to_bool,
                        nargs='?', const=True, help='add signatures to generator traffic')
    parser.add_argument('--fill', choices=['constant', 'increment', 'decrement', 'prbs'],
                        help="specify packet fill", default=None)
    parser.add_argument('--ports', nargs='+', type=str,
                        help='specify the transmit ports (create only)')
    parser.add_argument('--src_network', default='198.18.0.0/15', type=ipaddress.IPv4Network,
                        help='specify the traffic source network (create only)')
    parser.add_argument('--src_offset', default=1, type=int,
                        help='specify the index of the first source address (create only)')
    parser.add_argument('--dst_network', default='198.18.0.0/15', type=ipaddress.IPv4Network,
                        help='specify the traffic destination network (create only)')
    parser.add_argument('--dst_offset', default=65537, type=int,
                        help='specify the index of the first destination address (create only)')
    args = parser.parse_args()


    if args.fill and not args.signatures:
        args.signatures = True

    if args.action == 'create':
        create_generators(args)
    elif args.action == 'delete':
        delete_generators(args)
    elif args.action == 'start':
        start_generators(args)
    elif args.action == 'stop':
        stop_generators(args)
        aggregate_results(args)
    else:
        aggregate_results(args)


if __name__ == "__main__":
    main()
