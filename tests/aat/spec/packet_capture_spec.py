from mamba import description, before, after
from expects import *
import datetime
import os
import socket
import shutil
import subprocess
import tempfile
import time

import client.api
import client.models
from common import Config, Service
from common.helper import (get_capture_pcap, get_merged_capture_pcap)
from common.matcher import (be_valid_packet_capture,
                            be_valid_packet_capture_result,
                            raise_api_exception)
from common.helper import check_modules_exists

import scapy.all

ETH_HDR_SIZE = 14
IPV4_HDR_SIZE = 20
ICMP_HDR_SIZE = 8

CONFIG = Config(os.path.join(os.path.dirname(__file__),
                             os.environ.get('MAMBA_CONFIG', 'config.yaml')))

PING = subprocess.check_output('which ping'.split()).decode().rstrip()

def get_nth_port_id(api_client, index):
    ports_api = client.api.PortsApi(api_client)
    ports = ports_api.list_ports()
    expect(ports).not_to(be_empty)
    expect(len(ports) > index)
    return ports[index].id


def capture_model(api_client, id = None, buffer_size=16*1024*1024):
    if not id:
        id = get_nth_port_id(api_client, 0)

    config = client.models.PacketCaptureConfig(mode='buffer', buffer_size=buffer_size)
    capture = client.models.PacketCapture()
    capture.source_id = id
    capture.config = config

    return capture

def capture_models(api_client, buffer_size=16*1024*1024):
    cap1 = capture_model(api_client, get_nth_port_id(api_client, 0), buffer_size)
    cap2 = capture_model(api_client, get_nth_port_id(api_client, 1), buffer_size)
    return [cap1, cap2]

def get_interface_address(api_client, interface_id, domain):
    intf = api_client.get_interface(interface_id)
    if domain == socket.AF_INET:
        for protocol in intf.config.protocols:
            if protocol.ipv4:
                return protocol.ipv4.static.address
    elif domain == socket.AF_INET6:
        for protocol in intf.config.protocols:
            if protocol.ipv6:
                return protocol.ipv6.static.address
    else:
        raise AttributeError('Unsupported domain')

def ping_command(ping_binary, address, version=socket.AF_INET, count=1, payload_size=None):
    """Generate a ping shell command for Popen"""
    args = [ping_binary]

    if version == socket.AF_INET6:
        args.append('-6')
    else:
        args.append('-4')
    # ping interval
    args.append('-i .2')
    # ping count
    args.append('-c %d' % count)
    # payload size
    if payload_size:
        args.append('-s %d' % payload_size)

    args.append(address)
    return ' '.join(args).split()

def do_ping(api_client, ping_binary, src_id, dst_id, domain, count=1, payload_size=None):
    """Perform a ping from src to dst using the specified binary"""
    shim = Service(CONFIG.shim())
    dst_ip = get_interface_address(api_client, dst_id, domain)

    with open(os.devnull, 'w') as null:
        p = subprocess.Popen(ping_command(ping_binary, dst_ip, domain, count, payload_size),
                             stdout=null, stderr=null,
                             env={'LD_PRELOAD': shim.config.path,
                                  'OP_BINDTODEVICE': src_id})
        p.wait()
        expect(p.returncode).to(equal(0))

def get_pcap_with_wget(id, out_file):
    base_url = CONFIG.service('dataplane').base_url
    with open(os.devnull, 'w') as null:
        p = subprocess.Popen('wget -O %s %s/packet/capture-results/%s/pcap' % (out_file, base_url, id),
                             stdout=null, stderr=null, shell=True)
        p.wait()
        expect(p.returncode).to(equal(0))

def pcap_icmp_echo_request_count(pcap_file):
    count = 0
    icmp_type = scapy.all.ICMP(type='echo-request').type
    for packet in scapy.all.rdpcap(pcap_file):
        if 'ICMP' in packet and packet['ICMP'].type == icmp_type:
            count += 1
    return count

def pcap_icmp_echo_reply_count(pcap_file):
    count = 0
    icmp_type = scapy.all.ICMP(type='echo-reply').type
    for packet in scapy.all.rdpcap(pcap_file):
        if 'ICMP' in packet and packet['ICMP'].type == icmp_type:
            count += 1
    return count

def pcap_icmp_echo_request_lengths(pcap_file):
    lengths = []
    icmp_type = scapy.all.ICMP(type='echo-request').type
    for packet in scapy.all.rdpcap(pcap_file):
        if 'ICMP' in packet and packet['ICMP'].type == icmp_type:
            lengths.append(len(packet))
    return lengths

def pcap_icmp_echo_request_seq(pcap_file):
    seq = []
    icmp_type = scapy.all.ICMP(type='echo-request').type
    for packet in scapy.all.rdpcap(pcap_file):
        if 'ICMP' in packet:
            icmp = packet['ICMP']
            if icmp.type == icmp_type:
                seq.append(icmp.seq)
    return seq

def pcap_packet_timestamps(pcap_file):
    timestamps = []
    for packet in scapy.all.rdpcap(pcap_file):
        timestamps.append(packet.time)
    return timestamps

def validate_pcap_timestamps(pacp_file):
    timestamps = pcap_packet_timestamps(pacp_file)
    prev_ts = None
    for ts in timestamps:
        expect(ts).to(be_above(0))
        if (prev_ts):
            expect(ts).to(be_above_or_equal(prev_ts))
        prev_ts = ts


with description('Packet Capture,', 'packet_capture') as self:
    with description('REST API,'):

        with before.all:
            service = Service(CONFIG.service('dataplane'))
            self.process = service.start()
            self.api = client.api.PacketCapturesApi(service.client())
            self.intf_api = client.api.InterfacesApi(self.api.api_client)
            if not check_modules_exists(service.client(), 'packetio', 'packet-capture'):
                self.skip()

            # By default, ping is a privileged process.  We need it unprivileged
            # to use LD_PRELOAD, so just make a copy as a regular user.
            self.temp_dir = tempfile.mkdtemp()
            shutil.copy(PING, self.temp_dir)
            self.temp_ping = os.path.join(self.temp_dir, os.path.basename(PING))
            expect(os.path.isfile(self.temp_ping))

        with description('invalid HTTP methods,'):
            with description('/packet/captures,'):
                with it('returns 405'):
                    expect(lambda: self.api.api_client.call_api('/packet/captures', 'PUT')).to(
                        raise_api_exception(405, headers={'Allow': "DELETE, GET, POST"}))

            with description('/packet/capture-results,'):
                with it('returns 405'):
                    expect(lambda: self.api.api_client.call_api('/packet/capture-results', 'PUT')).to(
                        raise_api_exception(405, headers={'Allow': "DELETE, GET"}))

        with description('list captures,'):
            with before.each:
                cap = self.api.create_packet_capture(capture_model(self.api.api_client))
                expect(cap).to(be_valid_packet_capture)
                self.capture = cap

            with description('unfiltered,'):
                with it('succeeds'):
                    captures = self.api.list_packet_captures()
                    expect(captures).not_to(be_empty)
                    for cap in captures:
                        expect(cap).to(be_valid_packet_capture)

            with description('filtered,'):
                with description('by source_id,'):
                    with it('returns a capture'):
                        captures = self.api.list_packet_captures(source_id=self.capture.source_id)
                        expect(captures).not_to(be_empty)
                        for cap in captures:
                            expect(cap).to(be_valid_packet_capture)
                        expect([ cap for cap in captures if cap.id == self.capture.id ]).not_to(be_empty)

                with description('non-existent source_id,'):
                    with it('returns no captures'):
                        captures = self.api.list_packet_captures(source_id='foo')
                        expect(captures).to(be_empty)

        with description('get capture,'):
            with description('by existing capture id,'):
                with before.each:
                    cap = self.api.create_packet_capture(capture_model(self.api.api_client))
                    expect(cap).to(be_valid_packet_capture)
                    self.capture = cap

                with it('succeeds'):
                    expect(self.api.get_packet_capture(self.capture.id)).to(be_valid_packet_capture)

            with description('non-existent capture,'):
                with it('returns 404'):
                    expect(lambda: self.api.get_packet_capture('foo')).to(raise_api_exception(404))

            with description('invalid capture id,'):
                with it('returns 404'):
                    expect(lambda: self.api.get_packet_capture('foo')).to(raise_api_exception(404))

        with description('create capture,'):
            with description('empty source id,'):
                with it('returns 400'):
                    cap = capture_model(self.api.api_client)
                    cap.source_id = None
                    expect(lambda: self.api.create_packet_capture(cap)).to(raise_api_exception(400))

            with description('non-existent source id,'):
                with it('returns 400'):
                    cap = capture_model(self.api.api_client)
                    cap.source_id = 'foo'
                    expect(lambda: self.api.create_packet_capture(cap)).to(raise_api_exception(400))

        with description('delete capture,'):
            with description('by existing capture id,'):
                with before.each:
                    cap = self.api.create_packet_capture(capture_model(self.api.api_client))
                    expect(cap).to(be_valid_packet_capture)
                    self.capture = cap

                with it('succeeds'):
                    self.api.delete_packet_capture(self.capture.id)
                    expect(self.api.list_packet_captures()).to(be_empty)

            with description('non-existent capture id,'):
                with it('succeeds'):
                    self.api.delete_packet_capture('foo')

            with description('invalid capture id,'):
                with it('returns 404'):
                    expect(lambda: self.api.delete_packet_capture("invalid_id")).to(raise_api_exception(404))

        with description('start capture,'):
            with before.each:
                cap = self.api.create_packet_capture(capture_model(self.api.api_client))
                expect(cap).to(be_valid_packet_capture)
                self.capture = cap

            with description('by existing capture id,'):
                with it('succeeds'):
                    result = self.api.start_packet_capture(self.capture.id)
                    expect(result).to(be_valid_packet_capture_result)

            with description('non-existent capture id,'):
                with it('returns 404'):
                    expect(lambda: self.api.start_packet_capture('foo')).to(raise_api_exception(404))

        with description('start capture, buffer too large'):
            with it('returns 400'):
                # Try to allocate capture with 1 TB or memory
                cap = self.api.create_packet_capture(capture_model(self.api.api_client, buffer_size = 1024*1024*1024*1024))
                expect(cap).to(be_valid_packet_capture)
                self.capture = cap
                # Capture memory is allocated when the capture is started
                # so memory allocation failure occurs on start
                expect(lambda: self.api.start_packet_capture(self.capture.id)).to(raise_api_exception(400))

        with description('stop running capture,'):
            with before.each:
                cap = self.api.create_packet_capture(capture_model(self.api.api_client))
                expect(cap).to(be_valid_packet_capture)
                self.capture = cap
                result = self.api.start_packet_capture(self.capture.id)
                expect(result).to(be_valid_packet_capture_result)

            with description('by capture id,'):
                with it('succeeds'):
                    cap = self.api.get_packet_capture(self.capture.id)
                    expect(cap).to(be_valid_packet_capture)
                    expect(cap.active).to(be_true)

                    self.api.stop_packet_capture(self.capture.id)

                    cap = self.api.get_packet_capture(self.capture.id)
                    expect(cap).to(be_valid_packet_capture)
                    expect(cap.active).to(be_false)

        with description('list capture results,'):
            with before.each:
                cap = self.api.create_packet_capture(capture_model(self.api.api_client))
                expect(cap).to(be_valid_packet_capture)
                self.capture = cap
                result = self.api.start_packet_capture(self.capture.id)
                expect(result).to(be_valid_packet_capture_result)
                self.result = result

            with description('get result,'):
                with it('succeeds'):
                    result = self.api.get_packet_capture_result(id=self.result.id)
                    expect(result.id).to(equal(self.result.id))

            with description('unfiltered,'):
                with it('succeeds'):
                    results = self.api.list_packet_capture_results()
                    expect(results).not_to(be_empty)
                    for result in results:
                        expect(result).to(be_valid_packet_capture_result)

            with description('by capture id,'):
                with it('succeeds'):
                    results = self.api.list_packet_capture_results(capture_id=self.capture.id)
                    for result in results:
                        expect(result).to(be_valid_packet_capture_result)
                    expect([ r for r in results if r.capture_id == self.capture.id ]).not_to(be_empty)

            with description('non-existent capture id,'):
                with it('returns no results'):
                    results = self.api.list_packet_capture_results(capture_id='foo')
                    expect(results).to(be_empty)

            with description('by source id,'):
                with it('succeeds'):
                    results = self.api.list_packet_capture_results(source_id=get_nth_port_id(self.api.api_client, 0))
                    expect(results).not_to(be_empty)

            with description('non-existent source id,'):
                with it('returns no results'):
                    results = self.api.list_packet_capture_results(source_id='bar')
                    expect(results).to(be_empty)

        ###
        # XXX: Fill these in when we actually implement the code....
        ###
        with description('capture packets,'):
            with description('manual stop,'):
                with description('counters,'):
                    with it('has packets'):
                        cap = self.api.create_packet_capture(capture_model(self.api.api_client, 'dataplane-server'))
                        expect(cap).to(be_valid_packet_capture)
                        self.capture = cap

                        self.result = self.api.start_packet_capture(self.capture.id)
                        expect(self.result).to(be_valid_packet_capture_result)
                        do_ping(self.intf_api, self.temp_ping,
                                'dataplane-client', 'dataplane-server',
                                socket.AF_INET, 2)
                        self.api.stop_packet_capture(self.capture.id)
                        result = self.api.get_packet_capture_result(id=self.result.id)
                        expect(result.id).to(equal(self.result.id))
                        expect(result.packets).to(be_above_or_equal(2))

                with description('pcap,'):
                    with it('returns pcap'):
                        cap = self.api.create_packet_capture(capture_model(self.api.api_client, 'dataplane-server'))
                        expect(cap).to(be_valid_packet_capture)
                        self.capture = cap

                        self.result = self.api.start_packet_capture(self.capture.id)
                        expect(self.result).to(be_valid_packet_capture_result)
                        do_ping(self.intf_api, self.temp_ping,
                                'dataplane-client', 'dataplane-server',
                                socket.AF_INET, 4)
                        self.api.stop_packet_capture(self.capture.id)
                        result = self.api.get_packet_capture_result(id=self.result.id)
                        expect(result.id).to(equal(self.result.id))
                        expect(result.packets).to(be_above_or_equal(4))

                        out_file = os.path.join(self.temp_dir, 'test.pcapng')

                        # Retrieve PCAP using python API
                        get_capture_pcap(self.api, self.result.id, out_file)
                        expect(os.path.exists(out_file)).to(equal(True))
                        expect(pcap_icmp_echo_request_count(out_file)).to(equal(4))
                        os.remove(out_file)

                        # Retrieve PCAP using wget
                        get_pcap_with_wget(self.result.id, out_file)
                        expect(os.path.exists(out_file)).to(equal(True))
                        expect(pcap_icmp_echo_request_count(out_file)).to(equal(4))
                        os.remove(out_file)

                        # Make sure HTTP connection is still working
                        result = self.api.get_packet_capture_result(id=self.result.id)
                        expect(result.id).to(equal(self.result.id))
                        expect(result.packets).to(be_above_or_equal(4))

            with description('duration,'):
                with it('stops automatically'):
                    capture_duration_sec = 2
                    cap = capture_model(self.api.api_client, 'dataplane-server')
                    cap.config.duration = capture_duration_sec * 1000
                    cap = self.api.create_packet_capture(cap)
                    expect(cap).to(be_valid_packet_capture)
                    self.capture = cap

                    start_time = datetime.datetime.now()
                    self.result = self.api.start_packet_capture(self.capture.id)
                    expect(self.result).to(be_valid_packet_capture_result)
                    expect(self.result.state == 'started')
                    do_ping(self.intf_api, self.temp_ping,
                            'dataplane-client', 'dataplane-server',
                            socket.AF_INET, 4)
                    now = datetime.datetime.now()
                    while (now - start_time).total_seconds() < (capture_duration_sec + 2):
                        result = self.api.get_packet_capture_result(id=self.result.id)
                        if result.state == 'stopped':
                            break
                        time.sleep(.1)
                        now = datetime.datetime.now()
                    expect((now - start_time)).to(be_above_or_equal(datetime.timedelta(seconds=capture_duration_sec)))
                    if result.state != 'stopped':
                        # If it didn't stop on time this is an error
                        self.api.stop_packet_capture(self.capture.id)
                        expect(result.state == 'stopped')
                    expect(result.id).to(equal(self.result.id))
                    expect(result.packets).to(be_above_or_equal(4))

                    out_file = os.path.join(self.temp_dir, 'test.pcapng')

                    # Retrieve PCAP using python API
                    get_capture_pcap(self.api, self.result.id, out_file)
                    expect(os.path.exists(out_file)).to(equal(True))
                    expect(pcap_icmp_echo_request_count(out_file)).to(equal(4))
                    os.remove(out_file)

            with description('packet_count,'):
                with it('stops automatically'):
                    packet_count = 3
                    cap = capture_model(self.api.api_client, 'dataplane-server')
                    cap.config.packet_count = packet_count
                    cap = self.api.create_packet_capture(cap)
                    expect(cap).to(be_valid_packet_capture)
                    self.capture = cap

                    start_time = datetime.datetime.now()
                    self.result = self.api.start_packet_capture(self.capture.id)
                    expect(self.result).to(be_valid_packet_capture_result)
                    expect(self.result.state == 'started')
                    do_ping(self.intf_api, self.temp_ping,
                            'dataplane-client', 'dataplane-server',
                            socket.AF_INET, 5)
                    now = datetime.datetime.now()
                    while (now - start_time).total_seconds() < 5:
                        result = self.api.get_packet_capture_result(id=self.result.id)
                        if result.state == 'stopped':
                            break
                        time.sleep(.1)
                        now = datetime.datetime.now()
                    if result.state != 'stopped':
                        # If it didn't stop this is an error
                        self.api.stop_packet_capture(self.capture.id)
                        expect(result.state == 'stopped')
                    expect(result.id).to(equal(self.result.id))
                    expect(result.packets).to(equal(packet_count))

            with description('filter,'):
                with it('filters w/ icmp and length == 1000'):
                    packet_len = 1000
                    cap = capture_model(self.api.api_client, 'dataplane-server')
                    cap.config.filter = 'icmp and length == 1000'
                    cap = self.api.create_packet_capture(cap)
                    expect(cap).to(be_valid_packet_capture)
                    self.capture = cap

                    self.result = self.api.start_packet_capture(self.capture.id)
                    expect(self.result).to(be_valid_packet_capture_result)
                    expect(self.result.state == 'started')
                    # Send normal size packets
                    do_ping(self.intf_api, self.temp_ping,
                            'dataplane-client', 'dataplane-server',
                            socket.AF_INET, 2)
                    # Send large packets
                    do_ping(self.intf_api, self.temp_ping,
                            'dataplane-client', 'dataplane-server',
                            socket.AF_INET, 2, packet_len - ETH_HDR_SIZE - IPV4_HDR_SIZE - ICMP_HDR_SIZE)
                    self.api.stop_packet_capture(self.capture.id)
                    result = self.api.get_packet_capture_result(id=self.result.id)
                    expect(result.state == 'stopped')
                    expect(result.id).to(equal(self.result.id))
                    expect(result.packets).to(be_above_or_equal(2))

                    out_file = os.path.join(self.temp_dir, 'test.pcapng')

                    # Retrieve PCAP using python API
                    get_capture_pcap(self.api, self.result.id, out_file)
                    expect(os.path.exists(out_file)).to(equal(True))
                    lengths = pcap_icmp_echo_request_lengths(out_file)
                    expect(len(lengths)).to(equal(2))
                    for length in lengths:
                        expect(length).to(equal(packet_len))
                    os.remove(out_file)

            with description('partial pcap read,'):
                with it('succeeds'):
                    packet_count = 8
                    cap = capture_model(self.api.api_client, 'dataplane-server')
                    # Set filter to only all ICMP to make it easier to know what the packets are
                    cap.config.filter = 'icmp'
                    cap = self.api.create_packet_capture(cap)
                    expect(cap).to(be_valid_packet_capture)
                    self.capture = cap

                    self.result = self.api.start_packet_capture(self.capture.id)
                    expect(self.result).to(be_valid_packet_capture_result)
                    expect(self.result.state == 'started')

                    do_ping(self.intf_api, self.temp_ping,
                            'dataplane-client', 'dataplane-server',
                            socket.AF_INET, packet_count)
                    self.api.stop_packet_capture(self.capture.id)
                    result = self.api.get_packet_capture_result(id=self.result.id)
                    expect(result.state == 'stopped')
                    expect(result.id).to(equal(self.result.id))
                    expect(result.packets).to(be_above_or_equal(2))

                    out_file = os.path.join(self.temp_dir, 'test.pcapng')

                    expected_seq = list(range(1, packet_count + 1))

                    # Retrieve entire PCAP file
                    get_capture_pcap(self.api, self.result.id, out_file)
                    expect(os.path.exists(out_file)).to(equal(True))
                    seq = pcap_icmp_echo_request_seq(out_file)
                    expect(len(seq)).to(equal(8))
                    expect(seq).to(equal(expected_seq))
                    os.remove(out_file)

                    # Retrieve PCAP range 0 - 3
                    get_capture_pcap(self.api, self.result.id, out_file, 0, 3)
                    expect(os.path.exists(out_file)).to(equal(True))
                    seq = pcap_icmp_echo_request_seq(out_file)
                    expect(len(seq)).to(equal(4))
                    expect(seq).to(equal(expected_seq[0:4]))
                    os.remove(out_file)

                    # Retrieve PCAP range 3 - 6
                    get_capture_pcap(self.api, self.result.id, out_file, 3, 6)
                    expect(os.path.exists(out_file)).to(equal(True))
                    seq = pcap_icmp_echo_request_seq(out_file)
                    expect(len(seq)).to(equal(4))
                    expect(seq).to(equal(expected_seq[3:7]))
                    os.remove(out_file)

                    # Retrieve PCAP range 4 - 7
                    get_capture_pcap(self.api, self.result.id, out_file, 4, 7)
                    expect(os.path.exists(out_file)).to(equal(True))
                    seq = pcap_icmp_echo_request_seq(out_file)
                    expect(len(seq)).to(equal(4))
                    expect(seq).to(equal(expected_seq[4:8]))
                    os.remove(out_file)

            with description('start trigger,'):
                with it('triggers start w/ icmp and length == 1000'):
                    packet_len = 1000
                    cap = capture_model(self.api.api_client, 'dataplane-server')
                    cap.config.start_trigger = 'icmp and length == 1000'
                    cap = self.api.create_packet_capture(cap)
                    expect(cap).to(be_valid_packet_capture)
                    self.capture = cap

                    self.result = self.api.start_packet_capture(self.capture.id)
                    expect(self.result).to(be_valid_packet_capture_result)
                    expect(self.result.state == 'started')
                    # Send normal size packets
                    do_ping(self.intf_api, self.temp_ping,
                            'dataplane-client', 'dataplane-server',
                            socket.AF_INET, 2)
                    # Send large packets
                    do_ping(self.intf_api, self.temp_ping,
                            'dataplane-client', 'dataplane-server',
                            socket.AF_INET, 2, packet_len - ETH_HDR_SIZE - IPV4_HDR_SIZE - ICMP_HDR_SIZE)
                    self.api.stop_packet_capture(self.capture.id)
                    result = self.api.get_packet_capture_result(id=self.result.id)
                    expect(result.state == 'stopped')
                    expect(result.id).to(equal(self.result.id))
                    expect(result.packets).to(be_above_or_equal(2))

                    out_file = os.path.join(self.temp_dir, 'test.pcapng')

                    # Retrieve PCAP using python API
                    get_capture_pcap(self.api, self.result.id, out_file)
                    expect(os.path.exists(out_file)).to(equal(True))
                    lengths = pcap_icmp_echo_request_lengths(out_file)
                    expect(len(lengths)).to(equal(2))
                    for length in lengths:
                        expect(length).to(equal(packet_len))
                    os.remove(out_file)

            with description('stop trigger,'):
                with it('triggers stop w/ icmp and length == 1000'):
                    packet_len = 1000
                    cap = capture_model(self.api.api_client, 'dataplane-server')
                    cap.config.stop_trigger = 'icmp and length == 1000'
                    cap = self.api.create_packet_capture(cap)
                    expect(cap).to(be_valid_packet_capture)
                    self.capture = cap

                    self.result = self.api.start_packet_capture(self.capture.id)
                    expect(self.result).to(be_valid_packet_capture_result)
                    expect(self.result.state == 'started')
                    # Send normal size packets
                    do_ping(self.intf_api, self.temp_ping,
                            'dataplane-client', 'dataplane-server',
                            socket.AF_INET, 2)
                    # Send large packets
                    do_ping(self.intf_api, self.temp_ping,
                            'dataplane-client', 'dataplane-server',
                            socket.AF_INET, 2, packet_len - ETH_HDR_SIZE - IPV4_HDR_SIZE - ICMP_HDR_SIZE)
                    now = datetime.datetime.now()
                    start_time = now
                    while (now - start_time).total_seconds() < 5:
                        result = self.api.get_packet_capture_result(id=self.result.id)
                        if result.state == 'stopped':
                            break
                        time.sleep(.1)
                        now = datetime.datetime.now()
                    if result.state != 'stopped':
                        # If it didn't stop on time this is an error
                        self.api.stop_packet_capture(self.capture.id)
                        expect(result.state == 'stopped')
                    expect(result.id).to(equal(self.result.id))
                    expect(result.packets).to(be_above_or_equal(3))

                    out_file = os.path.join(self.temp_dir, 'test.pcapng')

                    # Retrieve PCAP using python API
                    get_capture_pcap(self.api, self.result.id, out_file)
                    expect(os.path.exists(out_file)).to(equal(True))
                    lengths = pcap_icmp_echo_request_lengths(out_file)
                    expect(len(lengths)).to(equal(3))
                    for length in lengths[0:-1]:
                        expect(length).not_to(equal(packet_len))
                    expect(lengths[-1]).to(equal(packet_len))
                    os.remove(out_file)

            with description('interface tx capture,'):
                with it('succeeds'):
                    cap = capture_model(self.api.api_client, 'dataplane-server', 1*1024*1024)
                    cap.direction = 'tx'
                    cap = self.api.create_packet_capture(cap)
                    expect(cap).to(be_valid_packet_capture)
                    self.capture = cap

                    self.result = self.api.start_packet_capture(self.capture.id)
                    expect(self.result).to(be_valid_packet_capture_result)
                    expect(self.result.state == 'started')
                    do_ping(self.intf_api, self.temp_ping,
                            'dataplane-client', 'dataplane-server',
                            socket.AF_INET, 2)
                    self.api.stop_packet_capture(self.capture.id)
                    result = self.api.get_packet_capture_result(id=self.result.id)
                    expect(result.state == 'stopped')
                    expect(result.id).to(equal(self.result.id))
                    expect(result.packets).to(be_above_or_equal(2))

                    out_file = os.path.join(self.temp_dir, 'test.pcapng')

                    # Retrieve PCAP using python API
                    get_capture_pcap(self.api, self.result.id, out_file)
                    expect(pcap_icmp_echo_request_count(out_file)).to(equal(0))
                    expect(pcap_icmp_echo_reply_count(out_file)).to(equal(2))
                    expect(os.path.exists(out_file)).to(equal(True))

                    validate_pcap_timestamps(out_file)

                    os.remove(out_file)


            with description('interface rx_and_tx capture,'):
                with it('succeeds'):
                    cap = capture_model(self.api.api_client, 'dataplane-server', 1*1024*1024)
                    cap.direction = 'rx_and_tx'
                    cap = self.api.create_packet_capture(cap)
                    expect(cap).to(be_valid_packet_capture)
                    self.capture = cap

                    self.result = self.api.start_packet_capture(self.capture.id)
                    expect(self.result).to(be_valid_packet_capture_result)
                    expect(self.result.state == 'started')
                    do_ping(self.intf_api, self.temp_ping,
                            'dataplane-client', 'dataplane-server',
                            socket.AF_INET, 2)
                    self.api.stop_packet_capture(self.capture.id)
                    result = self.api.get_packet_capture_result(id=self.result.id)
                    expect(result.state == 'stopped')
                    expect(result.id).to(equal(self.result.id))
                    expect(result.packets).to(be_above_or_equal(4))

                    out_file = os.path.join(self.temp_dir, 'test.pcapng')

                    # Retrieve PCAP using python API
                    get_capture_pcap(self.api, self.result.id, out_file)
                    expect(pcap_icmp_echo_request_count(out_file)).to(equal(2))
                    expect(pcap_icmp_echo_reply_count(out_file)).to(equal(2))
                    expect(os.path.exists(out_file)).to(equal(True))

                    validate_pcap_timestamps(out_file)

                    os.remove(out_file)

            with description('port rx_and_tx capture,'):
                with it('succeeds'):
                    # dataplane-server is on port0
                    cap = capture_model(self.api.api_client, 'port0', 1*1024*1024)
                    cap.direction = 'rx_and_tx'
                    cap = self.api.create_packet_capture(cap)
                    expect(cap).to(be_valid_packet_capture)
                    self.capture = cap

                    self.result = self.api.start_packet_capture(self.capture.id)
                    expect(self.result).to(be_valid_packet_capture_result)
                    expect(self.result.state == 'started')
                    do_ping(self.intf_api, self.temp_ping,
                            'dataplane-client', 'dataplane-server',
                            socket.AF_INET, 2)
                    self.api.stop_packet_capture(self.capture.id)
                    result = self.api.get_packet_capture_result(id=self.result.id)
                    expect(result.state == 'stopped')
                    expect(result.id).to(equal(self.result.id))
                    expect(result.packets).to(be_above_or_equal(4))

                    out_file = os.path.join(self.temp_dir, 'test.pcapng')

                    # Retrieve PCAP using python API
                    get_capture_pcap(self.api, self.result.id, out_file)
                    expect(pcap_icmp_echo_request_count(out_file)).to(equal(2))
                    expect(pcap_icmp_echo_reply_count(out_file)).to(equal(2))
                    expect(os.path.exists(out_file)).to(equal(True))

                    validate_pcap_timestamps(out_file)

                    os.remove(out_file)

            with description('rx + rx capture,'):
                with it('succeeds'):
                    server_cap = capture_model(self.api.api_client, 'dataplane-server', 1*1024*1024)
                    server_cap = self.api.create_packet_capture(server_cap)
                    expect(server_cap).to(be_valid_packet_capture)

                    client_cap = capture_model(self.api.api_client, 'dataplane-client', 1*1024*1024)
                    client_cap = self.api.create_packet_capture(client_cap)
                    expect(client_cap).to(be_valid_packet_capture)

                    server_result = self.api.start_packet_capture(server_cap.id)
                    expect(server_result).to(be_valid_packet_capture_result)
                    expect(server_result.state == 'started')

                    client_result = self.api.start_packet_capture(client_cap.id)
                    expect(client_result).to(be_valid_packet_capture_result)
                    expect(client_result.state == 'started')

                    do_ping(self.intf_api, self.temp_ping,
                            'dataplane-client', 'dataplane-server',
                            socket.AF_INET, 2)

                    self.api.stop_packet_capture(server_cap.id)
                    server_result = self.api.get_packet_capture_result(id=server_result.id)
                    expect(server_result.state == 'stopped')
                    expect(server_result.packets).to(be_above_or_equal(2))

                    self.api.stop_packet_capture(client_cap.id)
                    client_result = self.api.get_packet_capture_result(id=client_result.id)
                    expect(client_result.state == 'stopped')
                    expect(client_result.packets).to(be_above_or_equal(2))

                    out_file = os.path.join(self.temp_dir, 'test.pcapng')

                    # Retrieve PCAP using python API
                    get_merged_capture_pcap(self.api, [server_result.id, client_result.id], out_file)
                    expect(pcap_icmp_echo_request_count(out_file)).to(equal(2))
                    expect(pcap_icmp_echo_reply_count(out_file)).to(equal(2))
                    expect(os.path.exists(out_file)).to(equal(True))

                    validate_pcap_timestamps(out_file)

                    os.remove(out_file)

            with description('live,'):
                with _it('TODO'):
                    assert False

        with description('bulk operations,'):
            with description('bulk create,'):
                with description('valid request,'):
                    with it('succeeds'):
                        request = client.models.BulkCreatePacketCapturesRequest()
                        request.items = capture_models(self.api.api_client)
                        reply = self.api.bulk_create_packet_captures(request)
                        expect(reply.items).to(have_len(len(request.items)))
                        for item in reply.items:
                            expect(item).to(be_valid_packet_capture)

                with description('invalid requests,'):
                    with it('returns 400 for invalid config'):
                        request = client.models.BulkCreatePacketCapturesRequest()
                        request.items = capture_models(self.api.api_client)
                        request.items[-1].config.buffer_size = 1
                        expect(lambda: self.api.bulk_create_packet_captures(request)).to(raise_api_exception(400))
                        expect(self.api.list_packet_captures()).to(be_empty)

                    with it('returns 404 for invalid id'):
                        request = client.models.BulkCreatePacketCapturesRequest()
                        request.items = capture_models(self.api.api_client)
                        request.items[-1].id = 'cool::name'
                        expect(lambda: self.api.bulk_create_packet_captures(request)).to(raise_api_exception(404))
                        expect(self.api.list_packet_captures()).to(be_empty)

            with description('bulk delete,'):
                with before.each:
                    request = client.models.BulkCreatePacketCapturesRequest()
                    request.items = capture_models(self.api.api_client)
                    reply = self.api.bulk_create_packet_captures(request)
                    expect(reply.items).to(have_len(len(request.items)))
                    for item in reply.items:
                        expect(item).to(be_valid_packet_capture)

                with description('valid request,'):
                    with it('succeeds'):
                        self.api.bulk_delete_packet_captures(
                            client.models.BulkDeletePacketCapturesRequest(
                                [cap.id for cap in self.api.list_packet_captures()]))
                        expect(self.api.list_packet_captures()).to(be_empty)

                with description('invalid requests,'):
                    with it('succeeds with a non-existent id'):
                        self.api.bulk_delete_packet_captures(
                            client.models.BulkDeletePacketCapturesRequest(
                                [cap.id for cap in self.api.list_packet_captures()] + ['foo']))
                        expect(self.api.list_packet_captures()).to(be_empty)

                    with it('returns 404 for an invalid id'):
                        expect(lambda: self.api.bulk_delete_packet_captures(
                            client.models.BulkDeletePacketCapturesRequest(
                                [cap.id for cap in self.api.list_packet_captures()] + ['::bar']))).to(
                                    raise_api_exception(404))
                        expect(self.api.list_packet_captures()).not_to(be_empty)

            with description('bulk start,'):
                with before.each:
                    request = client.models.BulkCreatePacketCapturesRequest()
                    request.items = capture_models(self.api.api_client)
                    reply = self.api.bulk_create_packet_captures(request)
                    expect(reply.items).to(have_len(len(request.items)))
                    for item in reply.items:
                        expect(item).to(be_valid_packet_capture)

                with description('valid request'):
                    with it('succeeds'):
                        reply = self.api.bulk_start_packet_captures(
                            client.models.BulkStartPacketCapturesRequest(
                                [cap.id for cap in self.api.list_packet_captures()]))
                        expect(reply.items).to(have_len(len(self.api.list_packet_captures())))
                        for item in reply.items:
                            expect(item).to(be_valid_packet_capture_result)
                            expect(item.active).to(be_true)

                with description('invalid requests,'):
                    with it('returns 404 for non-existing id'):
                        expect(lambda: self.api.bulk_start_packet_captures(
                            client.models.BulkStartPacketCapturesRequest(
                                [cap.id for cap in self.api.list_packet_captures()] + ['foo']))).to(
                                    raise_api_exception(404))
                        for cap in self.api.list_packet_captures():
                            expect(cap.active).to(be_false)

                    with it('returns 404 for invalid id'):
                        expect(lambda: self.api.bulk_start_packet_captures(
                            client.models.BulkStartPacketCapturesRequest(
                                [cap.id for cap in self.api.list_packet_captures()] + ['::bar']))).to(
                                    raise_api_exception(404))
                        for cap in self.api.list_packet_captures():
                            expect(cap.active).to(be_false)

            with description('bulk stop,'):
                with before.each:
                    create_request = client.models.BulkCreatePacketCapturesRequest()
                    create_request.items = capture_models(self.api.api_client)
                    create_reply = self.api.bulk_create_packet_captures(create_request)
                    expect(create_reply.items).to(have_len(len(create_request.items)))
                    for item in create_reply.items:
                        expect(item).to(be_valid_packet_capture)
                    start_reply = self.api.bulk_start_packet_captures(
                        client.models.BulkStartPacketCapturesRequest(
                            [cap.id for cap in create_reply.items]))
                    expect(start_reply.items).to(have_len(len(create_request.items)))
                    for item in start_reply.items:
                        expect(item).to(be_valid_packet_capture_result)
                        expect(item.active).to(be_true)

                with description('valid request,'):
                    with it('succeeds'):
                        self.api.bulk_stop_packet_captures(
                            client.models.BulkStopPacketCapturesRequest(
                                [cap.id for cap in self.api.list_packet_captures()]))
                        captures = self.api.list_packet_captures()
                        expect(captures).not_to(be_empty)
                        for cap in captures:
                            expect(cap.active).to(be_false)

                with description('invalid requests,'):
                    with it('succeeds with a non-existent id'):
                        self.api.bulk_stop_packet_captures(
                            client.models.BulkStopPacketCapturesRequest(
                                [cap.id for cap in self.api.list_packet_captures()] + ['foo']))
                        captures = self.api.list_packet_captures()
                        expect(captures).not_to(be_empty)
                        for cap in captures:
                            expect(cap.active).to(be_false)

                    with it('returns 404 for an invalid id'):
                        expect(lambda: self.api.bulk_stop_packet_captures(
                            client.models.BulkStopPacketCapturesRequest(
                                [cap.id for cap in self.api.list_packet_captures()] + ['::bar']))).to(
                                    raise_api_exception(404))
                        captures = self.api.list_packet_captures()
                        expect(captures).not_to(be_empty)
                        for cap in captures:
                            expect(cap.active).to(be_true)

        with after.each:
            if hasattr(self, 'api'):
                for cap in self.api.list_packet_captures():
                    if cap.active:
                        self.api.stop_packet_capture(cap.id)
                self.api.delete_packet_captures()
            self.capture = None

        with after.all:
            if hasattr(self, 'process'):
                self.process.terminate()
                self.process.wait()
            if hasattr(self, 'temp_dir') and os.path.isdir(self.temp_dir):
                shutil.rmtree(self.temp_dir)
