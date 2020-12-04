#!/usr/bin/env python3

import argparse
import asyncore
import dpkt
import fcntl
import math
import os
import re
import socket
import struct
import sys
import time


# Various constants
ARPHRD_ETHER = 1
IFNAMSIZ = 16
SIOCGIFHWADDR = 0x8927
SIOCGIFADDR  = 0x8915
SIOCGIFINDEX = 0x8933

sockaddr_fmt = 'H14s'
sockaddr_in_fmt = 'HH4s'
sockaddr_in6_fmt = 'HHi16sI'
sockaddr_ll_fmt = 'HHiHBB8s'


def do_ifreq(ifname, cmd):
    s = socket.socket()
    try:
        if len(ifname) >= 16:
            raise RuntimeError("Interface name is too long to query with ioctls")

        return fcntl.ioctl(s, cmd, struct.pack('16s24x', ifname.encode('utf8')))[16:]
    finally:
        s.close()


def get_interface_ip_address(ifname):
    result = do_ifreq(ifname, SIOCGIFADDR)
    family,port,addr = struct.unpack(sockaddr_in_fmt, result[:struct.calcsize(sockaddr_in_fmt)])
    return socket.inet_ntoa(addr)


def get_interface_mac_address(ifname):
    result = do_ifreq(ifname, SIOCGIFHWADDR)
    family,data = struct.unpack(sockaddr_fmt, result[:struct.calcsize(sockaddr_fmt)])
    return data[0:6]


def get_interface_index(ifname):
    result = do_ifreq(ifname, SIOCGIFINDEX)
    idx = struct.unpack('i', result[:struct.calcsize('i')])
    return idx[0]


def mac_to_bytes(mac):
    return bytes(map(lambda x: int(x, 16), re.split(':|-|\.', mac)))


def to_sockaddr_ll(ifname, mac):
    return struct.pack(sockaddr_ll_fmt, socket.AF_PACKET,0,get_interface_index(ifname),0,0,8,mac_to_bytes(mac))


def get_ping_packet(seq, interface_id, dest_ip, dest_mac=None):
    echo = dpkt.icmp.ICMP.Echo(
        id = os.getpid() % 65535,
        seq = seq,
        data = b'ABCDEFGHIJKLMNOPQRSTUVWXYZ')

    icmp = dpkt.icmp.ICMP(
        type = dpkt.icmp.ICMP_ECHO,
        data = echo)

    ip = dpkt.ip.IP(
        src = socket.inet_aton(get_interface_ip_address(interface_id)),
        dst = socket.inet_aton(dest_ip),
        ttl = 64,
        p = dpkt.ip.IP_PROTO_ICMP,
        data = icmp)

    if not dest_mac:
        return ip

    packet = dpkt.ethernet.Ethernet(
        src = get_interface_mac_address(interface_id),
        dst = mac_to_bytes(dest_mac),
        type = dpkt.ethernet.ETH_TYPE_IP,
        data = ip)

    return packet


def match_ping_packet(data, interface_id, src_ip, src_mac=None):
    no_match = (False, None, None)

    if src_mac:
        eth = dpkt.ethernet.Ethernet(data)
        if eth.src != mac_to_bytes(src_mac):
            return no_match
        ip = eth.data
    else:
        ip = dpkt.ip.IP(data)

    if ip.src != socket.inet_aton(src_ip):
        return no_match

    if ip.dst != socket.inet_aton(get_interface_ip_address(interface_id)):
        return no_match

    if ip.p != dpkt.ip.IP_PROTO_ICMP:
        return no_match

    icmp = ip.data

    if icmp.type != dpkt.icmp.ICMP_ECHOREPLY:
        return no_match

    echo = icmp.data

    if echo.id != os.getpid() % 65535:
        return no_match

    return True, echo.seq, ip.ttl


class Pinger(asyncore.dispatcher):
    def __init__(self, ifname, dest_ip, dest_mac=None, tx_limit=None):
        asyncore.dispatcher.__init__(self)
        self.socket_type = socket.SOCK_RAW if dest_mac else socket.SOCK_DGRAM
        self.ifname = ifname
        self.dest_ip = dest_ip
        self.dest_mac = dest_mac
        self.tx_limit = tx_limit
        self.tx_count = 0
        self.rx_count = 0
        self.tx_times = []
        self.round_trip_times = []
        self.create_socket()

    def writable(self):
        # Can write every second up to tx limit
        if self.tx_limit and self.tx_count > self.tx_limit:
            return False

        if len(self.tx_times) == 0:
            return True

        if time.time() - self.tx_times[-1] > 1:
            return True

        return False

    def handle_write(self):
        if self.tx_limit and self.tx_count == self.tx_limit:
            self.close()
            return

        seq = self.tx_count + 1
        self.tx_times.append(time.time())
        packet = get_ping_packet(seq, self.ifname, self.dest_ip, self.dest_mac)
        result = self.send(bytes(packet))
        if not result:
            sys.stderr.write('Send failed!\n')
            sys.stderr.flush()
        else:
            self.tx_count += 1

    def readable(self):
        return not self.writable()

    def handle_read(self):
        now = time.time()
        packet = self.recv(4096)
        match,seq,ttl = match_ping_packet(packet, self.ifname, self.dest_ip, self.dest_mac)
        if match:
            self.rx_count += 1
            ms = (now - self.tx_times[seq - 1]) * 1000
            print('{} bytes from {}: icmp_seq={} ttl={} time={:.3f} ms'.format(
                len(packet), self.dest_ip, seq, ttl, ms))
            self.round_trip_times.append(ms)

            if self.tx_limit and self.rx_count >= self.tx_limit:
                self.close()

    def handle_connect(self):
        pass

    def handle_accept(self):
        pass

    def handle_close(self):
        self.close()

    def handle_error(self):
        # People who think it's a good idea to swallow exceptions should be
        # flayed, quartered, and burned.
        raise

    def create_socket(self):
        s = socket.socket(socket.AF_PACKET, self.socket_type, socket.htons(dpkt.ethernet.ETH_TYPE_IP))
        s.setsockopt(socket.SOL_SOCKET, socket.SO_BINDTODEVICE,
                     struct.pack('%ds' % (len(self.ifname) + 1), bytes(self.ifname, 'utf-8')))
        self.socket = s
        self.set_socket(s)

    def send(self, data):
        if self.dest_mac:
            return self.socket.sendto(data, (self.ifname, socket.htons(dpkt.ethernet.ETH_TYPE_IP)))
        else:
            # You get a packet! And you get a packet!
            # Everybody gets a packet!!
            return self.socket.sendto(data, (self.ifname,
                                             socket.htons(dpkt.ethernet.ETH_TYPE_IP),
                                             0,
                                             ARPHRD_ETHER,
                                             mac_to_bytes('ff:ff:ff:ff:ff:ff')))


def do_ping(args):
    start = time.time()

    try:
        pinger = Pinger(args.interface, args.destination, args.mac, args.count)
        asyncore.loop(.5)

    except KeyboardInterrupt:
        pass

    else:
        runtime = time.time() - start

        # Print statistics
        print()
        print('--- {} ping statistics ---'.format(args.destination))
        print('{} packets transmitted, {} packets received, {:.0f}% packet loss, time {:.0f}ms'.format(
            pinger.tx_count, pinger.rx_count,
            100 * (pinger.tx_count - pinger.rx_count) / pinger.tx_count if pinger.tx_count else 0,
            runtime * 1000))

        if pinger.rx_count:
            # Calculate stddev of round trip times; ping calls this mdev
            tmp1 = sum(pinger.round_trip_times) / pinger.rx_count
            tmp2 = sum(map(lambda x: x*x, pinger.round_trip_times)) / pinger.rx_count
            mdev = math.sqrt(tmp2 - (tmp1 * tmp1))

            print('rtt min/avg/max/mdev = {:.3f}/{:.3f}/{:.3f}/{:.3f} ms'.format(
                min(pinger.round_trip_times),
                sum(pinger.round_trip_times) / len(pinger.round_trip_times),
                max(pinger.round_trip_times),
                mdev))


def main():
    parser = argparse.ArgumentParser(description='ping utility using AF_PACKET sockets')
    parser.add_argument('-c', '--count', nargs='?', dest='count', type=int)
    parser.add_argument('-M', '--mac', nargs='?')
    parser.add_argument('interface')
    parser.add_argument('destination')

    do_ping(parser.parse_args())


if __name__ == "__main__":
    main()
