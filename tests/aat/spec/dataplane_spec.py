import os
import random
import select
import shutil
import socket
import string
import subprocess
import sys
import tempfile

import client.api

from expects import *
from common import Config, Service

###
# Spec constants
###
CONFIG = Config(os.path.join(os.path.dirname(__file__),
                             os.environ.get('MAMBA_CONFIG', 'config.yaml')))
PING = subprocess.check_output('which ping'.split()).decode().rstrip()
NC = subprocess.check_output('which nc'.split()).decode().rstrip()

POLL_TIMEOUT = 5 * 1000               # poll timeout in ms
BULK_TRANSMIT_SIZE = 4 * 1024 * 1024  #  4 MB
TCP_TRANSMIT_CHUNK = 16 * 1024        # 16 kB
UDP_TRANSMIT_CHUNK = 1024             #  1 kB


# Grade UDP on a curve
def expected_transmit(protocol):
    if protocol == socket.IPPROTO_UDP:
        return BULK_TRANSMIT_SIZE / 2

    return BULK_TRANSMIT_SIZE


class nc_command_impl(object):
    """Simple class to parse command line options for nc"""

    def __init__(self, address, **kwargs):
        self.address = address
        self.listen = kwargs['listen'] if 'listen' in kwargs else False
        self.port = kwargs['port'] if 'port' in kwargs else 3357
        self.protocol = kwargs['protocol'] if 'protocol' in kwargs else socket.IPPROTO_TCP
        self.verbose = kwargs['verbose'] if 'verbose' in kwargs else False
        self.version = kwargs['version'] if 'version' in kwargs else socket.AF_INET

        if self.version != socket.AF_INET and self.version != socket.AF_INET6:
            raise AttributeError('Invalid IP version')

    def __repr__(self):
        args = [NC]

        if self.protocol == socket.IPPROTO_UDP:
            args.append('-u')

        if self.version == socket.AF_INET:
            args.append('-4')
        elif self.version == socket.AF_INET6:
            args.append('-6')

        if self.listen:
            args.append('-l')

        if self.verbose:
            args.append('-v')

        args.append('-n')  # Don't perform any name lookups
        args.append(self.address)
        args.append(str(self.port))

        return ' '.join(args)


def nc_command(address, **kwargs):
    """Generate a nc shell command for Popen"""
    cmd = nc_command_impl(address, **kwargs)
    return str(cmd).split()


def ping_command(ping_binary, address, version=socket.AF_INET):
    """Generate a ping shell command for Popen"""
    args = [ping_binary]

    if version == socket.AF_INET6:
        args.append('-6')
    else:
        args.append('-4')

    # One ping only
    args.append('-c 1')

    args.append(address)

    return ' '.join(args).split()


def is_server_interface(interface_id):
    return interface_id.find('server') >= 0


def get_interface_address(api_client, interface_id, domain):
    intf = api_client.get_interface(interface_id)
    if domain == socket.AF_INET:
        return intf.config.protocols[-1].ipv4.static.address
    else:
        raise AttributeError('Unsupported domain')


def wait_for_keyword(output_stream, keyword):
    """Wait for the keyword on the output stream"""

    found = False
    poller = select.poll()
    poller.register(output_stream, select.POLLIN | select.POLLHUP | select.POLLERR)

    while not found:
        events = poller.poll(POLL_TIMEOUT)

        if not len(events):
            raise RuntimeError("output stream timeout")

        for fd, event in events:
            if event & select.POLLIN:
                data = os.read(fd, 1024)
                data = data.decode()
                if data.find(keyword) >= 0:
                    found = True
            else:
                raise RuntimeError("Unexpected output stream event = {}".format(event))


def wait_until_writable(input_stream):
    """Wait until the input stream is writable"""

    writable = False
    poller = select.poll()
    poller.register(input_stream, select.POLLOUT)

    while not writable:
        events = poller.poll(POLL_TIMEOUT)

        if not len(events):
            raise RuntimeError("input stream timeout")

        for fd, event in events:
            if event & select.POLLOUT:
                writable = True
            else:
                raise RuntimeError("Unexpected input stream event = {}".format(event))


def create_connected_endpoints(api_client, reader_id, writer_id, domain, protocol, null):
    """
    We need to create the server endpoint before the client endpoint, otherwise we
    run the risk of the client failing to connect before the server is started
    This function juggles the order as appropriate and returns the reader/writer
    subprocesses running the nc instances.  It also waits for verification that
    the client process has connected before returning.
    """

    shim = Service(CONFIG.shim())
    reader = None
    writer = None

    server_id = reader_id if is_server_interface(reader_id) else writer_id
    server_ip_addr = get_interface_address(api_client, server_id, domain)

    server_input = None
    if is_server_interface(writer_id):
        writer = subprocess.Popen(nc_command(server_ip_addr, protocol=protocol, listen=True),
                                  stdin=subprocess.PIPE,
                                  stdout=null,
                                  stderr=subprocess.PIPE,
                                  close_fds=True,
                                  env={'LD_PRELOAD': shim.config.path,
                                       'OP_BINDTODEVICE': writer_id})
        server_input = writer.stdin

    if is_server_interface(reader_id):
        reader = subprocess.Popen(nc_command(server_ip_addr, protocol=protocol, listen=True),
                                  stdin=subprocess.PIPE,
                                  stdout=subprocess.PIPE,
                                  stderr=subprocess.PIPE,
                                  close_fds=True,
                                  env={'LD_PRELOAD': shim.config.path,
                                       'OP_BINDTODEVICE': reader_id})
        server_input = reader.stdin

    try:
        # Use a writable stdin as a proxy for a listening server.  Using
        # the verbose option to nc can trigger a getnameinfo() call, which
        # can fail inside a container.
        wait_until_writable(server_input)
    except Exception as e:
        if reader: reader.kill()
        if writer: writer.kill()
        raise e

    client_output = None
    if not is_server_interface(writer_id):
        writer = subprocess.Popen(nc_command(server_ip_addr, protocol=protocol, verbose=True),
                                  stdin=subprocess.PIPE,
                                  stdout=null,
                                  stderr=subprocess.PIPE,
                                  close_fds=True,
                                  env={'LD_PRELOAD': shim.config.path,
                                       'OP_BINDTODEVICE': writer_id})
        client_output = writer.stderr

    if not is_server_interface(reader_id):
        reader = subprocess.Popen(nc_command(server_ip_addr, protocol=protocol, verbose=True),
                                  stdin=null,
                                  stdout=subprocess.PIPE,
                                  stderr=subprocess.PIPE,
                                  close_fds=True,
                                  env={'LD_PRELOAD': shim.config.path,
                                       'OP_BINDTODEVICE': reader_id})
        client_output = reader.stderr

    try:
        wait_for_keyword(client_output, "succeeded")
    except Exception as e:
        reader.kill()
        writer.kill()
        raise e

    return reader, writer


def do_bulk_data_transfer(api_client, reader_id, writer_id, domain, protocol):
    """Perform a bulk data transfer from writer to reader using nc"""
    io_size = BULK_TRANSMIT_SIZE
    io_chunk = TCP_TRANSMIT_CHUNK if protocol == socket.IPPROTO_TCP else UDP_TRANSMIT_CHUNK

    # We need to pipe some of the subprocess output to /dev/null to prevent
    # seeing it.
    with open(os.devnull, 'w') as null:
        reader, writer = create_connected_endpoints(api_client,
                                                    reader_id, writer_id,
                                                    domain, protocol,
                                                    null)
        total_read = 0
        total_written = 0

        poller = select.poll()
        poller.register(reader.stdout, select.POLLIN | select.POLLHUP | select.POLLERR)
        poller.register(writer.stdin, select.POLLOUT)

        try:
            while total_read < expected_transmit(protocol):
                events = poller.poll(POLL_TIMEOUT)

                if not len(events):
                    raise RuntimeError("IO timeout read={}, write={}".format(total_read, total_written))

                for fd, event in events:
                    if event & select.POLLIN:
                        total_read += len(os.read(fd, io_chunk))
                    elif event & select.POLLOUT:
                        length = min(max(io_size - total_written, 0), io_chunk)
                        data = ''.join(random.choice(string.printable) for _ in range(length))
                        total_written += os.write(fd, data.encode())
                        if total_written == io_size:
                            poller.unregister(fd)
                    else:
                        raise RuntimeError("Unexpected IO event = {}".format(event))

        except Exception as e:
            raise e

        finally:
            reader.terminate()
            writer.terminate()

        return total_read, total_written


def get_interface_counter(api_client, interface_id, counter):
    return getattr(api_client.get_interface(interface_id).stats, counter)


def do_ping(api_client, ping_binary, src_id, dst_id, domain):
    """Perform a ping from src to dst using the specified binary"""
    shim = Service(CONFIG.shim())
    dst_ip = get_interface_address(api_client, dst_id, domain)

    with open(os.devnull, 'w') as null:
        p = subprocess.Popen(ping_command(ping_binary, dst_ip),
                             stdout=null, stderr=null,
                             env={'LD_PRELOAD': shim.config.path,
                                  'OP_BINDTODEVICE': src_id})
        p.wait()
        expect(p.returncode).to(equal(0))


with description('Dataplane,', 'dataplane') as self:

    with before.all:
        service = Service(CONFIG.service('dataplane'))
        self.process = service.start()
        self.api = client.api.InterfacesApi(service.client())

    with description('ipv4,'):
        with description('ping,'):
            with before.all:
                # By default, ping is a privileged process.  We need it unprivileged
                # to use LD_PRELOAD, so just make a copy as a regular user.
                self.temp_dir = tempfile.mkdtemp()
                shutil.copy(PING, self.temp_dir)
                self.temp_ping = os.path.join(self.temp_dir, os.path.basename(PING))
                expect(os.path.isfile(self.temp_ping))

            with description('client interface,'):
                with it('succeeds'):
                    do_ping(self.api, self.temp_ping,
                            'dataplane-client', 'dataplane-server',
                            socket.AF_INET)

            with description('server interface,'):
                with it('succeeds'):
                    do_ping(self.api, self.temp_ping,
                            'dataplane-server', 'dataplane-client',
                            socket.AF_INET)

            with after.all:
                shutil.rmtree(self.temp_dir)

        with description('tcp,'):
            with description('bulk data transfer,'):
                with description('client to server,'):
                    with it('succeeds'):
                        server_rx_start = get_interface_counter(self.api,
                                                                'dataplane-server',
                                                                'rx_bytes')
                        client_tx_start = get_interface_counter(self.api,
                                                                'dataplane-client',
                                                                'tx_bytes')

                        read, written = do_bulk_data_transfer(self.api,
                                                              'dataplane-server',
                                                              'dataplane-client',
                                                              socket.AF_INET,
                                                              socket.IPPROTO_TCP)

                        server_rx_stop = get_interface_counter(self.api,
                                                               'dataplane-server',
                                                               'rx_bytes')
                        client_tx_stop = get_interface_counter(self.api,
                                                               'dataplane-client',
                                                               'tx_bytes')

                        expect(read).to(equal(BULK_TRANSMIT_SIZE))
                        expect(written).to(equal(BULK_TRANSMIT_SIZE))

                        expect(server_rx_stop - server_rx_start) \
                            .to(be_above_or_equal(BULK_TRANSMIT_SIZE))
                        expect(client_tx_stop - client_tx_start) \
                            .to(be_above_or_equal(BULK_TRANSMIT_SIZE))

                with description('server to client,'):
                    with it('succeeds'):
                        server_tx_start = get_interface_counter(self.api,
                                                                'dataplane-server',
                                                                'tx_bytes')
                        client_rx_start = get_interface_counter(self.api,
                                                                'dataplane-client',
                                                                'rx_bytes')

                        read, written = do_bulk_data_transfer(self.api,
                                                              'dataplane-client',
                                                              'dataplane-server',
                                                              socket.AF_INET,
                                                              socket.IPPROTO_TCP)

                        server_tx_stop = get_interface_counter(self.api,
                                                               'dataplane-server',
                                                               'tx_bytes')
                        client_rx_stop = get_interface_counter(self.api,
                                                               'dataplane-client',
                                                               'rx_bytes')

                        expect(read).to(equal(BULK_TRANSMIT_SIZE))
                        expect(written).to(equal(BULK_TRANSMIT_SIZE))

                        expect(client_rx_stop - client_rx_start) \
                            .to(be_above_or_equal(BULK_TRANSMIT_SIZE))
                        expect(server_tx_stop - server_tx_start) \
                            .to(be_above_or_equal(BULK_TRANSMIT_SIZE))

        with description('udp,'):
            with description('bulk data transfer,'):
                with description('client to server,'):
                    with it('succeeds,'):
                        server_rx_start = get_interface_counter(self.api,
                                                                'dataplane-server',
                                                                'rx_bytes')
                        client_tx_start = get_interface_counter(self.api,
                                                                'dataplane-client',
                                                                'tx_bytes')

                        read, written = do_bulk_data_transfer(self.api,
                                                              'dataplane-server',
                                                              'dataplane-client',
                                                              socket.AF_INET,
                                                              socket.IPPROTO_UDP)

                        server_rx_stop = get_interface_counter(self.api,
                                                               'dataplane-server',
                                                               'rx_bytes')
                        client_tx_stop = get_interface_counter(self.api,
                                                               'dataplane-client',
                                                               'tx_bytes')

                        # Note: `nc` uses up to 16k for buffering input data.
                        # However, that much data will not fit in a single UDP
                        # packet.  Currently, the stack doesn't fragment outgoing
                        # packets, so we can lose some data, even when testing
                        # back to back across ring devices.
                        # Note 2: When using UDP, `nc` sends probe packets, so we can
                        # read more data that we sent, but that's ok.
                        expect(read).to(be_above_or_equal(expected_transmit(socket.IPPROTO_UDP)))
                        expect(written).to(be_above_or_equal(expected_transmit(socket.IPPROTO_UDP)))

                        expect(server_rx_stop - server_rx_start) \
                            .to(be_above_or_equal(read))
                        expect(client_tx_stop - client_tx_start) \
                            .to(be_above_or_equal(written))

                with description('server to client,'):
                    with it('succeeds'):
                        server_tx_start = get_interface_counter(self.api,
                                                                'dataplane-server',
                                                                'tx_bytes')
                        client_rx_start = get_interface_counter(self.api,
                                                                'dataplane-client',
                                                                'rx_bytes')

                        read, written = do_bulk_data_transfer(self.api,
                                                              'dataplane-client',
                                                              'dataplane-server',
                                                              socket.AF_INET,
                                                              socket.IPPROTO_UDP)

                        server_tx_stop = get_interface_counter(self.api,
                                                               'dataplane-server',
                                                               'tx_bytes')
                        client_rx_stop = get_interface_counter(self.api,
                                                               'dataplane-client',
                                                               'rx_bytes')

                        expect(read).to(be_above_or_equal(expected_transmit(socket.IPPROTO_UDP)))
                        expect(written).to(be_above_or_equal(expected_transmit(socket.IPPROTO_UDP)))

                        expect(client_rx_stop - client_rx_start) \
                            .to(be_above_or_equal(read))
                        expect(server_tx_stop - server_tx_start) \
                            .to(be_above_or_equal(written))

    with after.all:
        try:
            self.process.terminate()
            self.process.wait()
        except AttributeError:
            pass
