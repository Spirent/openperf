#!/usr/bin/env python3

import argparse
import json
import ipaddress
import pprint
import random
import re
import requests


###
# Pistache appears to have a limit on the size of JSON data included with
# HTTP requests.  These values specify the number of items to include in
# the bulk-create/bulk-delete requests and are experimentally determined,
# e.g. they worked for me.
###
ADD_PAGE_SIZE = 12
DEL_PAGE_SIZE = 32

def get_random_mac(port_id):
    octets = list()
    octets.append(random.randint(0, 255) & 0xfc)
    octets.append((port_id >> 16) & 0xff)
    octets.append(port_id & 0xff)
    for _i in range(3):
        octets.append(random.randint(0, 255))

    return '{0:02x}:{1:02x}:{2:02x}:{3:02x}:{4:02x}:{5:02x}'.format(*octets)


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
        'id': '{}-{}'.format(tag, index),
        'config': {
            'protocols': [ eth_protocol, ip_protocol ],
        }
    }


def add_interfaces(url, tag, port_id, network, count, offset):
    items = list()
    for idx in range(1 + offset, 1 + offset + count):
        items.append(get_static_interface_config(tag, port_id, network, idx))

    total = 0
    while total < count:
        r = requests.post("/".join([url, 'interfaces/x/bulk-create']),
                          json={'items': items[total:total + ADD_PAGE_SIZE]})
        r.raise_for_status()
        total += ADD_PAGE_SIZE


def del_interfaces(url, tag, count):
    # Retrieve the list of interface ids from the inception instance
    get = requests.get("/".join([url, 'interfaces']))
    get.raise_for_status()
    # Note: key provides a natural sort order for id's,
    # e.g. tag-2 comes after tag-1 and before tag-10.
    del_candidates = sorted(map(lambda inf: inf['id'],
                                filter(lambda inf: inf['id'].startswith(tag), get.json())),
                            key = lambda s: [int(x) if x.isdigit() else x.lower() for x in re.split('(\d+)', s)])
    total = 0
    while total < count:
        # Unlike with additions, it is NOT ok to go past the end of the list
        expunge = requests.post("/".join([url, 'interfaces/x/bulk-delete']),
                                json={'ids': del_candidates[total:min(total + DEL_PAGE_SIZE, count)]})
        expunge.raise_for_status()
        total += DEL_PAGE_SIZE


def main():
    parser = argparse.ArgumentParser(description='Bulk interface testing for inception')
    parser.add_argument('url')
    parser.add_argument('action', choices=['add', 'remove'])
    parser.add_argument('--tag', default='scalability', type=str,
                        help='prefix for interface ids')
    parser.add_argument('--count', default=1, type=int,
                        help='the number of interfaces to add/delete')
    parser.add_argument('--port', default='port0', type=str,
                        help='specify the interface port (add only)')
    parser.add_argument('--network', default='198.18.0.0/15', type=ipaddress.IPv4Network,
                        help='specify the interface network (add only)')
    parser.add_argument('--offset', default=0, type=int,
                        help='specify the index of the first network host address (add only)')
    args = parser.parse_args()

    if args.action == 'add':
        add_interfaces(args.url, args.tag, args.port, args.network, args.count, args.offset)
    else:
        del_interfaces(args.url, args.tag, args.count)


if __name__ == "__main__":
    main()
