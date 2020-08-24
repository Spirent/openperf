from mamba import description, before, after, it
from expects import *
import os

import client.api
import client.models
from common import Config, Service
from common.matcher import be_valid_port, raise_api_exception
from common.helper import check_modules_exists


CONFIG = Config(os.path.join(os.path.dirname(__file__), os.environ.get('MAMBA_CONFIG', 'config.yaml')))


with description('Ports,', 'ports') as self:
    with description('REST API,'):

        with before.all:
            service = Service(CONFIG.service())
            self.process = service.start()
            self.api = client.api.PortsApi(service.client())
            if not check_modules_exists(service.client(), 'packetio'):
                self.skip()

        with description('list,'):
            with description('unfiltered,'):
                with it('returns valid ports'):
                    ports = self.api.list_ports()
                    expect(ports).not_to(be_empty)
                    for port in ports:
                        expect(port).to(be_valid_port)

            with description('filtered,'):
                with description('known existing kind,'):
                    with it('returns valid ports of that kind'):
                        ports = self.api.list_ports(kind='dpdk')
                        expect(ports).not_to(be_empty)
                        for port in ports:
                            expect(port).to(be_valid_port)
                            expect(port.kind).to(equal('dpdk'))

                with description('known non-existent kind,'):
                    with it('returns no ports'):
                        ports = self.api.list_ports(kind='host')
                        expect(ports).to(be_empty)

                with description('invalid kind,'):
                    with it('returns no ports'):
                        ports = self.api.list_ports(kind='foo')
                        expect(ports).to(be_empty)

                with description('missing kind,'):
                    with it('returns no ports'):
                        ports = self.api.list_ports(kind='')
                        expect(ports).to(be_empty)

            with description('unsupported method,'):
                with it('returns 405'):
                    expect(lambda: self.api.api_client.call_api('/ports', 'PUT')).to(raise_api_exception(
                        405, headers={'Allow': "GET, POST"}))

        with description('get,'):
            with description('known existing port,'):
                with it('succeeds'):
                    expect(self.api.get_port('0')).to(be_valid_port)

            with description('non-existent port,'):
                with it('returns 404'):
                    expect(lambda: self.api.get_port('foo')).to(raise_api_exception(404))

            with description('invalid id,'):
                with it('returns 404'):
                    expect(lambda: self.api.get_port('Inv_port')).to(raise_api_exception(404))

            with description('unsupported method,'):
                with it('returns 405'):
                    expect(lambda: self.api.api_client.call_api('/ports/0', 'POST')).to(raise_api_exception(
                        405, headers={'Allow': "PUT, DELETE, GET"}))

        with description('create,'):
            with before.each:
                self.port = client.models.Port()

            with description('dpdk kind,'):
                with before.each:
                    self.port.kind = 'dpdk'
                    self.port.config = client.models.PortConfig(dpdk=client.models.PortConfigDpdk())

                with it('returns 400'):
                    expect(lambda: self.api.delete_port('0')).to(raise_api_exception(400))

            with description('host kind,'):
                with before.each:
                    self.port.kind ='host'

                with it('returns 400'):
                    expect(lambda: self.api.delete_port('0')).to(raise_api_exception(400))

            with description('bond kind,'):
                with before.each:
                    self.port.kind = 'bond'
                    self.port.config = client.models.PortConfig(bond=client.models.PortConfigBond())

                with description('LAG 802.3AD mode,'):
                    with before.each:
                        self.port.config.bond.mode = 'lag_802_3_ad'

                    with description('valid port list,'):
                        with before.each:
                            ports = self.api.list_ports(kind='dpdk')
                            expect(ports).not_to(be_empty)
                            self.port.config.bond.ports = [ p.id for p in ports ]
                            expect(self.port.config.bond.ports).not_to(be_empty)

                        with it('succeeds'):
                            port = self.api.create_port(self.port)
                            expect(port).to(be_valid_port)
                            self.cleanup = port
                            # TODO: no bond type in back end, at present
                            #expect(port.kind).to(equal(self.port.kind))
                            #expect(port.config.bond.mode).to(equal(self.port.config.bond.mode))

                    with description('invalid port list,'):
                        with before.each:
                            self.port.config.bond.ports = ['foo']

                        with it('returns 400'):
                            expect(lambda: self.api.create_port(self.port)).to(raise_api_exception(400))

                    with description('empty port list,'):
                        with before.each:
                            self.port.config.bond.ports = []

                        with it('returns 400'):
                            expect(lambda: self.api.create_port(self.port)).to(raise_api_exception(400))

                    with description('missing port list,'):
                        with it('returns 400'):
                            expect(lambda: self.api.create_port(self.port)).to(raise_api_exception(400))

                    with description('invalid member port id,'):
                        with before.each:
                            self.port.config.bond.ports = ['Invalid_port']

                        with it('returns 400'):
                            expect(lambda: self.api.create_port(self.port)).to(raise_api_exception(400))

                with description('invalid mode,'):
                    with before.each:
                        self.port.config.bond.mode = 'foo'

                    with it('returns 400'):
                        expect(lambda: self.api.create_port(self.port)).to(raise_api_exception(400))

                with description('missing mode,'):
                    with it('returns 400'):
                        expect(lambda: self.api.create_port(self.port)).to(raise_api_exception(400))

            with description('invalid kind,'):
                with before.each:
                    self.port.kind = 'foo'

                with it('returns 400'):
                    expect(lambda: self.api.create_port(self.port)).to(raise_api_exception(400))

            with description('missing kind,'):
                with it('returns 400'):
                    expect(lambda: self.api.create_port(self.port)).to(raise_api_exception(400))

            with description('invalid ID,'):
                with before.each:
                    self.port.id = 'Invalid_id'

                with it ('returns 400'):
                    expect(lambda: self.api.create_port(self.port)).to(raise_api_exception(400))

        with description('delete,'):
            with description('valid port,'):
                with before.each:
                    ports = self.api.list_ports(kind='dpdk')
                    self.port = client.models.Port()
                    self.port.kind = 'bond'
                    self.port.config = client.models.PortConfig(bond=client.models.PortConfigBond())
                    self.port.config.bond.mode = 'lag_802_3_ad'
                    self.port.config.bond.ports = [ p.id for p in ports ]
                    expect(self.port.config.bond.ports).not_to(be_empty)
                    port = self.api.create_port(self.port)
                    expect(port).to(be_valid_port)
                    self.port, self.cleanup = port, port

                with it('succeeds'):
                    self.api.delete_port(self.port.id)
                    expect(lambda: self.api.get_port(self.port.id)).to(raise_api_exception(404))
                    self.cleanup = None

            with description('protected port,'):
                with it('returns 400'):
                    expect(lambda: self.api.delete_port('0')).to(raise_api_exception(400))

            with description('non-existent port,'):
                with it('succeeds'):
                    self.api.delete_port('foo')

            with description('invalid port ID,'):
                with it('returns 404'):
                    expect(lambda: self.api.delete_port('Invalid_port_id')).to(raise_api_exception(404))

        with after.each:
            try:
                if self.cleanup is not None:
                    try:
                        self.api.delete_port(self.cleanup.id)
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

    with description ('configuration file,'):
        with before.all:
           service = Service(CONFIG.service('config-file'))
           self.process = service.start()
           self.api = client.api.PortsApi(service.client())

        with it('created a valid port'):
            port = self.api.get_port('port-bond')
            expect(port).to(be_valid_port)
            # TODO: no bond type in back end, at present
            #expect(port.kind).to(equal(self.port.kind))
            #expect(port.config.bond.mode).to(equal(self.port.config.bond.mode))

        with after.all:
            try:
                self.process.terminate()
                self.process.wait()
            except AttributeError:
                pass
