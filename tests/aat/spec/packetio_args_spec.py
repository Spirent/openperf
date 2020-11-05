from mamba import description, before, after, it
from expects import expect
import os

import client.api
from common import Config, Service
from common.helper import check_modules_exists


CONFIG = Config(os.path.join(os.path.dirname(__file__), os.environ.get('MAMBA_CONFIG', 'config.yaml')))


with description('PacketIO arguments,', 'packetio_args'):
    with description('Empty core mask,'):
        with before.all:
            service = Service(CONFIG.service('packetio-args-1'))
            self.process = service.start()
            self.api = client.api.PortsApi(service.client())
            if not check_modules_exists(service.client(), 'packetio'):
                self.skip()

        with description('binary started,'):
            with it('has no port handlers'):
                expect(lambda: self.api.list_ports().to(raise_api_exception(405)))

        with after.all:
            try:
                self.process.terminate()
                self.process.wait()
            except AttributeError:
                pass

    with description('No-init flag,'):
        with before.all:
            service = Service(CONFIG.service('packetio-args-2'))
            self.process = service.start()
            self.api = client.api.PortsApi(service.client())
            if not check_modules_exists(service.client(), 'packetio'):
                self.skip()

        with description('binary started,'):
            with it('has no port handlers'):
                expect(lambda: self.api.list_ports().to(raise_api_exception(405)))

        with after.all:
            try:
                self.process.terminate()
                self.process.wait()
            except AttributeError:
                pass

    with description('No stack worker,'):
        with before.all:
            service = Service(CONFIG.service('packetio-args-3'))
            self.process = service.start()
            self.api = client.api.StacksApi(service.client())
            if not check_modules_exists(service.client(), 'packet-stack'):
                self.skip()

        with description('binary started,'):
            with it('has no stack handlers'):
                expect(lambda: self.api.list_stacks().to(raise_api_exception(405)))

        with after.all:
            try:
                self.process.terminate()
                self.process.wait()
            except AttributeError:
                pass
