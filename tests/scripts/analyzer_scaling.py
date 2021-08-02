#!/usr/bin/env python3

import argparse
import datetime
import os
import requests


###
# Pistache appears to have a limit on the size of JSON data included with
# HTTP requests.  These values specify the number of items to include in
# the bulk-create/bulk-delete requests and are experimentally determined,
# e.g. they worked for me.
###
ADD_PAGE_SIZE = 12
ID_PAGE_SIZE = 32


def get_all_ports(url):
    get = requests.get("/".join([url, 'ports']))
    get.raise_for_status()
    return [p['id'] for p in get.json()]


def get_analyzer_config(idx, port, counters, digests, tag):
    config = {'flow_counters': counters,
              'flow_digests': digests,
              'protocol_counters': ['ethernet', 'ip', 'transport']}
    return {'id': '{}-{}'.format(tag, idx),
            'config': config,
            'source_id': port}


def create_analyzers(args):
    if not args.ports:
        args.ports = get_all_ports(args.url)
    print(args.ports)

    configs = list()
    for idx, port in enumerate(args.ports):
        configs.append(get_analyzer_config(idx, port, args.counters, args.digests, args.tag))

    assert len(configs) > 0, 'No analyzer configs!'

    total = 0
    while total < len(configs):
        r = requests.post("/".join([args.url, 'packet/analyzers/x/bulk-create']),
                          json={'items': configs[total:total + ADD_PAGE_SIZE]})
        r.raise_for_status()
        total += ADD_PAGE_SIZE


def get_analyzer_ids(args):
    get = requests.get("/".join([args.url, 'packet/analyzers']))
    get.raise_for_status()
    return [ana['id'] for ana in filter(lambda ana: ana['id'].startswith(args.tag), get.json())]


def do_bulk_analyzer_request(args, op):
    ids = get_analyzer_ids(args)
    total = 0
    count = len(ids)
    while total < count:
        result = requests.post("/".join([args.url, 'packet/analyzers/x', op]),
                               json={'ids': ids[total:min(total + ID_PAGE_SIZE, count)]})
        result.raise_for_status()
        total += ID_PAGE_SIZE

def delete_analyzers(args):
    do_bulk_analyzer_request(args, 'bulk-delete')


def start_analyzers(args):
    do_bulk_analyzer_request(args, 'bulk-start')


def get_result_ids(args):
    # Retrieve the list of generator ids from the openperf instance
    get = requests.get("/".join([args.url, 'packet/analyzer-results']))
    get.raise_for_status()
    return [result['id'] for result in filter(
        lambda result: result['analyzer_id'].startswith(args.tag), get.json())]


def iso8601_to_datetime(s):
    # Python's datetime can't handle nanoseconds, so strip them off
    return datetime.datetime.strptime(s[:-4], '%Y-%m-%dT%H:%M:%S.%f')


def aggregate_results(args):
    octets = 0
    packets = 0
    start = None
    stop = None

    for handle in get_result_ids(args):
        r = requests.get('/'.join([args.url, 'packet/analyzer-results', handle]))
        counters = r.json()['flow_counters']
        packets += counters['frame_count']
        if 'frame_length' in counters:
            octets += counters['frame_length']['summary']['total']
        if 'timestamp_first' in counters:
            local_start = iso8601_to_datetime(counters['timestamp_first'])
            local_stop = iso8601_to_datetime(counters['timestamp_last'])
            start = local_start if not start else min(start, local_start)
            stop = local_stop if not stop else max(stop, local_stop)

    if start and stop:
        delta = (stop - start).total_seconds()
        if octets:
            print('{} packets ({} octets) in {} seconds ({:.2f} pps, {:.2f} octets/sec)'.format(
                packets, octets, delta,
                packets / delta if delta else 0,
                octets / delta if delta else 0))
        else:
            print('{} packets in {} seconds ({:.2f} pps)'.format(
                packets, delta, packets / delta if delta else 0))
    else:
        print('No results!')


def stop_analyzers(args):
    do_bulk_analyzer_request(args, 'bulk-stop')
    aggregate_results(args)


def main():
    parser = argparse.ArgumentParser(description='Bulk analyzer testing for openperf')

    if 'URL' not in os.environ:
        parser.add_argument('url')
    else:
        parser.add_argument('--url', default=os.environ.get('URL'))

    parser.add_argument('action', choices=['create', 'delete', 'start', 'stop'])
    parser.add_argument('--tag', default='analyzer', type=str,
                        help='perfix for analyzer ids')
    parser.add_argument('--ports', nargs='+', type=str,
                        help='ports to use for analyzers; create only')
    parser.add_argument('--counters', nargs='+',
                        choices=['advanced_sequencing',
                                 'frame_length',
                                 'interarrival_time',
                                 'jitter_ipdv',
                                 'jitter_rfc',
                                 'latency',
                                 'prbs',
                                 'header'],
                        default=[],
                        help='counters to enable on analyzers; create only')
    parser.add_argument('--digests', nargs='+',
                        choices=['frame_length',
                                 'interarrival_time',
                                 'jitter_ipdv',
                                 'jitter_rfc',
                                 'latency',
                                 'sequence_run_length'],
                        default=[],
                        help='digests to enable on analyzers; create only')
    args=parser.parse_args()

    if 'frame_count' not in args.counters:
        args.counters.append('frame_count')

    action_map = {
        'create': create_analyzers,
        'delete': delete_analyzers,
        'start': start_analyzers,
        'stop': stop_analyzers
    }

    action_map[args.action](args)


if __name__ == "__main__":
    main()
