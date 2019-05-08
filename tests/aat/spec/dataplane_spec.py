from mamba import description, before, after, it
from expects import *

import os
import random
import select
import shlex
import subprocess
from threading import Thread

import client.api
import client.models
from common import Config, Service
from common.helper import (ipv4_interface)
from common.matcher import be_valid_interface, raise_api_exception

CONFIG = Config(os.path.join(os.path.dirname(__file__), os.environ.get('MAMBA_CONFIG', 'config.yaml')))

def runlstnr(command, intf, shim, trace):
    p = subprocess.Popen(shlex.split(command),
                         stdin=None, stdout=subprocess.PIPE, stderr=None, close_fds=True,
                         env={'LD_PRELOAD': shim,
                              'ICP_BINDTODEVICE': 'io{}'.format(intf),
                              'ICP_TRACE': trace})
    return p

def runwrtr(command, intf, shim, trace, lstnr):
    for i in range(10):
        p = subprocess.Popen(shlex.split(command),
                             stdin=subprocess.PIPE, stdout=None, stderr=None, close_fds=True,
                             env={'LD_PRELOAD': shim,
                                  'ICP_BINDTODEVICE': 'io{}'.format(intf),
                                  'ICP_TRACE': trace})
        sendstr = 'spirent handshake string'
        p.stdin.write(sendstr)
        p.stdin.flush()
        r, w, e = select.select([lstnr.stdout], [], [], 1)
        if lstnr.stdout in r:
            recvstr = os.read(r[0].fileno(), len(sendstr))
            if recvstr == sendstr:
                return 0, p
        p.kill()
        p.wait()
    return -1, None

def tcpsenddata(procs, datalen):
    datarecv = datasend = ''
    random.seed()
    for i in range(datalen):
        datasend += chr(random.randint(ord('!'), ord('~')))
    procs[1].stdin.write(datasend)
    procs[1].stdin.flush()
    while True:
        r, w, e = select.select([procs[0].stdout], [], [], 10)
        if len(r) == 0:
            return False
        datarecv += os.read(r[0].fileno(), datalen)
        if len(datarecv) == datalen: 
            break;
    return datarecv == datasend

def udpsenddata(procs, datalen):
    datarecv = datasend = ''
    random.seed()
    for i in range(datalen):
        datasend += chr(random.randint(ord('!'), ord('~')))
    maxsend = 1000
    iters = datalen / maxsend
    for i in range(iters):
        procs[1].stdin.write(datasend[i * maxsend:i * maxsend + maxsend])
        procs[1].stdin.flush()
        while True:
            r, w, e = select.select([procs[0].stdout], [], [], 10)
            if len(r) == 0:
                return False
            datarecv += os.read(r[0].fileno(), maxsend)
            if len(datarecv) == (i * maxsend + maxsend):
                break;
    return datarecv == datasend

with description('dataplane,') as self:
    with before.all:
        service = Service(CONFIG.service())
        self.process = service.start()
        self.api = client.api.InterfacesApi(service.client())
        self.service = service
        self.shim = Service(CONFIG.shim())

    with description('ipv4,'):
        with before.each:
            self.intfs = []
            self.procs = []
            self.cleanup = []
            self.addresses = [ '10.0.0.{}'.format(i+1) for i in range(2) ]
            self.intfconfigs = [ ipv4_interface(self.api.api_client,
                                                ipv4_address=self.addresses[i]) for i in range(2) ]

            for i in range(2):
                self.intfconfigs[i].port_id = str(i)
                intf = self.api.create_interface(self.intfconfigs[i])
                expect(intf).to(be_valid_interface)
                self.intfs.append(intf)
                self.cleanup.append(intf)

        with description('tcp,'):
            with description('data transfer'):
                with before.each:
                    helper = Service(CONFIG.helper('nc-ipv4-tcp-lstnr'))
                    command = helper.config.command.format(address=self.addresses[0])
                    p = runlstnr(command, self.intfs[0].id, self.shim.config.path,
                                 self.shim.config.trace)
                    self.procs.append(p)
                    helper = Service(CONFIG.helper('nc-ipv4-tcp-wrtr'))
                    command = helper.config.command.format(address=self.addresses[0])
                    y, p = runwrtr(command, self.intfs[1].id, self.shim.config.path,
                                   self.shim.config.trace, self.procs[0])
                    expect(y).to(equal(0))
                    self.procs.append(p)

                with description('sendrecv'):
                    with it('succeeds'):
                        y = tcpsenddata(self.procs, 10000)
                        expect(y).to(equal(True))

                with after.each:
                    if self.procs is not None:
                        for proc in self.procs:
                            proc.kill()
                            proc.wait()

        with description('udp,'):
            with description('data transfer'):
                with before.each:
                    helper = Service(CONFIG.helper('nc-ipv4-udp-lstnr'))
                    command = helper.config.command.format(address=self.addresses[0])
                    p = runlstnr(command, self.intfs[0].id, self.shim.config.path,
                                 self.shim.config.trace)
                    self.procs.append(p)
                    helper = Service(CONFIG.helper('nc-ipv4-udp-wrtr'))
                    command = helper.config.command.format(address=self.addresses[0])
                    y, p = runwrtr(command, self.intfs[1].id, self.shim.config.path,
                                   self.shim.config.trace, self.procs[0])
                    expect(y).to(equal(0))
                    self.procs.append(p)

                with description('sendrecv'):
                    with it('succeeds'):
                        y = udpsenddata(self.procs, 10000)
                        expect(y).to(equal(True))

                with after.each:
                    if self.procs is not None:
                        for proc in self.procs:
                            proc.kill()
                            proc.wait()

        with after.each:
            try:
                if self.cleanup is not None:
                    for intf in self.cleanup:
                        try:
                            self.api.delete_interface(intf.id)
                        except:
                            pass
                    self.cleanup = None
            except AttributeError:
                pass

    with after.all:
        try:
            self.process.kill()
            self.process.wait()
        except AttributeError:
            pass
