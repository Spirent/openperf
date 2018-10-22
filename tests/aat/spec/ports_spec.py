from mamba import description, before, after, it
from expects import *
import os

import client.api
import client.models
from common import Config, Service
from matchers import be_valid_port, raise_api_exception


CONFIG = Config(os.path.join(os.path.dirname(__file__), 'config.yaml'))


with description('Ports,') as self:
    with before.all:
        service = Service(CONFIG.service())
        self.process = service.start()
        self.ports = client.api.PortsApi(service.client())

    with description('list,'):
        with it('succeeds'):
            ps = self.ports.list_ports()
            expect(ps).not_to(be_empty)
            [ expect(p).to(be_valid_port) for p in ps ]

    with description('get,'):
        with description('valid port,'):
            with it('succeeds'):
                expect(self.ports.get_port('0')).to(be_valid_port)

        with description('non-existent port,'):
            with it('returns 404'):
                expect(lambda: self.ports.get_port('foo')).to(raise_api_exception(404))

    with description('create,'):
        with before.each:
            self.port = client.models.Port()

        with description('dpdk kind,'):
            with before.each:
                self.port.kind = 'dpdk'
                self.port.config = client.models.PortConfig(dpdk=client.models.PortConfigDpdk())

            with it('returns 400'):
                expect(lambda: self.ports.delete_port('0')).to(raise_api_exception(400))

        with description('host kind,'):
            with before.each:
                self.port.kind ='host'

            with it('returns 400'):
                expect(lambda: self.ports.delete_port('0')).to(raise_api_exception(400))

        with description('bond kind,'):
            with before.each:
                self.port.kind = 'bond'
                self.port.config = client.models.PortConfig(bond=client.models.PortConfigBond())

            with description('LAG 802.3AD mode,'):
                with before.each:
                    self.port.config.bond.mode = 'lag_802_3_ad'

                with description('valid port list,'):
                    with before.each:
                        ps = self.ports.list_ports() # TODO: filter?
                        expect(ps).not_to(be_empty)
                        self.port.config.bond.ports = [ p.id for p in ps ]

                    with it('succeeds'):
                        p = self.ports.create_port(self.port)
                        expect(p).to(be_valid_port)
                        self.cleanup = p
                        # TODO: no bond type in back end, at present
                        #expect(p.kind).to(equal(self.port.kind))
                        #expect(p.config.bond.mode).to(equal(self.port.config.bond.mode))

                with description('invalid port list,'):
                    with before.each:
                        self.port.config.bond.ports = ['foo']

                    with it('returns 400'):
                        expect(lambda: self.ports.create_port(self.port)).to(raise_api_exception(400))

                with description('empty port list,'):
                    with before.each:
                        self.port.config.bond.ports = []

                    with it('returns 400'):
                        expect(lambda: self.ports.create_port(self.port)).to(raise_api_exception(400))

                with description('missing port list,'):
                    with it('returns 400'):
                        expect(lambda: self.ports.create_port(self.port)).to(raise_api_exception(400))

            with description('invalid mode,'):
                with before.each:
                    self.port.config.bond.mode = 'foo'

                with it('returns 400'):
                    expect(lambda: self.ports.create_port(self.port)).to(raise_api_exception(400))

            with description('missing mode,'):
                with it('returns 400'):
                    expect(lambda: self.ports.create_port(self.port)).to(raise_api_exception(400))

        with description('invalid kind,'):
            with before.each:
                self.port.kind = 'foo'

            with it('returns 400'):
                expect(lambda: self.ports.create_port(self.port)).to(raise_api_exception(400))

        with description('missing kind,'):
            with it('returns 400'):
                expect(lambda: self.ports.create_port(self.port)).to(raise_api_exception(400))

    with description('delete,'):
        with description('valid port,'):
            with before.each:
                ps = self.ports.list_ports() # TODO: filter?
                self.port = client.models.Port()
                self.port.kind = 'bond'
                self.port.config=client.models.PortConfig(bond=client.models.PortConfigBond())
                self.port.config.bond.mode = 'lag_802_3_ad'
                self.port.config.bond.ports = [ p.id for p in ps ]
                p = self.ports.create_port(self.port)
                expect(p).to(be_valid_port)
                self.cleanup = p

            with it('succeeds'):
                self.ports.delete_port(self.cleanup.id)

        with description('protected port,'):
            with it('returns 400'):
                expect(lambda: self.ports.delete_port('0')).to(raise_api_exception(400))

        with description('non-existent port,'):
            with it('succeeds'):
                self.ports.delete_port('foo')

    with after.each:
        try:
            if self.cleanup is not None:
                try:
                    self.ports.delete_port(self.cleanup.id)
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
