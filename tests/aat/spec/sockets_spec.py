from mamba import description, before, after, it
from expects import *
import os
import sys

import client.api
import client.models
from common import Config, Service
from common.helper import (ipv4_interface)
from common.matcher import be_valid_interface, raise_api_exception


CONFIG = Config(os.path.join(os.path.dirname(__file__), os.environ.get('MAMBA_CONFIG', 'config.yaml')))

with description('Sockets,') as self:
    with before.all:
        service = Service(CONFIG.service())
        self.process = service.start()
        self.api = client.api.InterfacesApi(service.client())
        self.n = 0

    with description('tcp,'):
        with before.each:
            self.addresses = [ '10.0.0.{}'.format(self.n+i+1) for i in range(2) ]
            self.intfconfigs = [ ipv4_interface(self.api.api_client, ipv4_address=self.addresses[i]) for i in range(2) ]
            self.intfs = []
            self.cleanup = []
            for i in range(2):
                self.intfconfigs[i].port_id = str(i)
                intf = self.api.create_interface(self.intfconfigs[i])
                expect(intf).to(be_valid_interface)
                self.intfs.append(intf)
                self.cleanup.append(intf)
                self.n+=1

        with description('handshake,'):
            with it('succeeds'):
                error = False
                try:
                    pid = os.fork()
                    if pid == 0:
                        os.execve(sys.executable, ['', './scripts/tcphandshake.py', self.addresses[0], '9999', 'io{}'.format(self.intfs[0].id), 'io{}'.format(self.intfs[1].id)], {'ICP_TRACE': '0', 'LD_PRELOAD': '../../build/libicp-shim-linux-x86_64-testing/lib/libicp-shim.so'})
                        os._exit(-1)
                    else:
                        pid, stat = os.waitpid(pid, 0)
                        expect(stat).to(equal(0))
                except:
                    error = True
                expect(error).to(equal(False))

        with description('sendrecv,'):
            with it('succeeds'):
                error = False
                try:
                    pid = os.fork()
                    if pid == 0:
                        os.execvp('sudo', ['', 'ICP_TRACE=0', 'LD_PRELOAD=../../build/libicp-shim-linux-x86_64-testing/lib/libicp-shim.so', sys.executable, './scripts/tcpsendrecv.py', self.addresses[0], '9999', 'io{}'.format(self.intfs[0].id), 'io{}'.format(self.intfs[1].id)])
                        os._exit(-1)
                    else:
                        pid, stat = os.waitpid(pid, 0)
                        expect(stat).to(equal(0))
                except:
                    error = True
                expect(error).to(equal(False))

    with after.each:
        try:
            if self.cleanup is not None:
                if not hasattr(self.cleanup, '__iter__'):
                    self.cleanup = [ self.cleanup ]
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
            self.process.terminate()
            self.process.wait()
        except AttributeError:
            pass
