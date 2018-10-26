from mamba import description, before, after, it
from expects import *
import os

import client.api
import client.models
from common import Config, Service
from common.helper import example_interface
from common.matcher import be_valid_interface, raise_api_exception


CONFIG = Config(os.path.join(os.path.dirname(__file__), 'config.yaml'))


with description('Interfaces,') as self:
    with before.all:
        service = Service(CONFIG.service())
        self.process = service.start()
        self.interfaces = client.api.InterfacesApi(service.client())

    with description('list,'):
        with before.each:
            i = self.interfaces.create_interface(example_interface(self.interfaces.api_client))
            expect(i).to(be_valid_interface)
            self.intf, self.cleanup = i, i

        with description('unfiltered,'):
            with it('succeeds'):
                _is = self.interfaces.list_interfaces()
                expect(_is).not_to(be_empty)
                for i in _is:
                    expect(i).to(be_valid_interface)
                expect([ i for i in _is if i.id == self.intf.id ]).not_to(be_empty)

        with description('filtered,'):
            with description('port with existing interfaces,'):
                with it('returns at least one interface'):
                    _is = self.interfaces.list_interfaces(port_id=self.intf.port_id)
                    expect([ i for i in _is if i.id == self.intf.id ]).not_to(be_empty)

            with description('invalid port,'):
                with it('returns no interfaces'):
                    _is = self.interfaces.list_interfaces(port_id='foo')
                    expect(_is).to(be_empty)

            with description('missing port,'):
                with it('returns no interfaces'):
                    _is = self.interfaces.list_interfaces(port_id='')
                    expect(_is).to(be_empty)

            with description('known existing Ethernet MAC address,'):
                with it('returns exactly one interface'):
                    _is = self.interfaces.list_interfaces(eth_mac_address=self.intf.config.protocols[0].eth.mac_address)
                    expect(_is).to(have_len(1))
                    expect(_is[0].id).to(equal(self.intf.id))

            with description('invalid Ethernet MAC address,'):
                with it('returns no interfaces'):
                    _is = self.interfaces.list_interfaces(eth_mac_address='foo')
                    expect(_is).to(be_empty)

            with description('missing Ethernet MAC address,'):
                with it('returns no interfaces'):
                    _is = self.interfaces.list_interfaces(eth_mac_address='')
                    expect(_is).to(be_empty)

            with description('known existing IPv4 address,'):
                with it('returns exactly one interface'):
                    _is = self.interfaces.list_interfaces(ipv4_address=self.intf.config.protocols[1].ipv4.static.address)
                    expect(_is).to(have_len(1))
                    expect(_is[0].id).to(equal(self.intf.id))

            with description('invalid IPv4 address,'):
                with it('returns no interfaces'):
                    _is = self.interfaces.list_interfaces(ipv4_address='foo')
                    expect(_is).to(be_empty)

            with description('missing IPv4 address,'):
                with it('returns no interfaces'):
                    _is = self.interfaces.list_interfaces(ipv4_address='')
                    expect(_is).to(be_empty)

    with description('get,'):
        with description('known existing interface,'):
            with before.each:
                i = self.interfaces.create_interface(example_interface(self.interfaces.api_client))
                expect(i).to(be_valid_interface)
                self.intf, self.cleanup = i, i

            with it('succeeds'):
                expect(self.interfaces.get_interface(self.intf.id)).to(be_valid_interface)

        with description('non-existent interface,'):
            with it('returns 404'):
                expect(lambda: self.interfaces.get_interface('foo')).to(raise_api_exception(404))

    with description('create,'):
        pass

    with description('delete,'):
        with description('valid interface,'):
            with before.each:
                i = self.interfaces.create_interface(example_interface(self.interfaces.api_client))
                expect(i).to(be_valid_interface)
                self.intf, self.cleanup = i, i

            with it('succeeds'):
                self.interfaces.delete_interface(self.intf.id)
                expect(lambda: self.interfaces.get_interface(self.intf.id)).to(raise_api_exception(404))
                self.cleanup = None

        with description('non-existent interface,'):
            with it('succeeds'):
                self.interfaces.delete_interface('foo')

    with after.each:
        try:
            if self.cleanup is not None:
                try:
                    self.interfaces.delete_interface(self.cleanup.id)
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
