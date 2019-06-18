from mamba import description, before, after, it
from expects import *
import os

import client.api
import client.models
from common import Config, Service
from common.helper import (empty_interface,
                           example_interface,
                           as_interface_protocol,
                           ipv4_interface)
from common.matcher import be_valid_interface, raise_api_exception


CONFIG = Config(os.path.join(os.path.dirname(__file__), os.environ.get('MAMBA_CONFIG', 'config.yaml')))
BULK_OP_SIZE = 4


with description('Interfaces,') as self:
    with description ('REST API,'):

        with before.all:
            service = Service(CONFIG.service())
            self.process = service.start()
            self.api = client.api.InterfacesApi(service.client())

        with description('list,'):
            with before.each:
                intf = self.api.create_interface(example_interface(self.api.api_client))
                expect(intf).to(be_valid_interface)
                self.intf, self.cleanup = intf, intf

            with description('unfiltered,'):
                with it('succeeds'):
                    intfs = self.api.list_interfaces()
                    expect(intfs).not_to(be_empty)
                    for intf in intfs:
                        expect(intf).to(be_valid_interface)
                        expect([ i for i in intfs if i.id == self.intf.id ]).not_to(be_empty)

            with description('filtered,'):
                with description('port with existing interfaces,'):
                    with it('returns at least one interface'):
                        intfs = self.api.list_interfaces(port_id=self.intf.port_id)
                        expect([ i for i in intfs if i.id == self.intf.id ]).not_to(be_empty)

                with description('non-existent port,'):
                    with it('returns no interfaces'):
                        intfs = self.api.list_interfaces(port_id='foo')
                        expect(intfs).to(be_empty)

                with description('missing port,'):
                    with it('returns no interfaces'):
                        intfs = self.api.list_interfaces(port_id='')
                        expect(intfs).to(be_empty)

                with description('known existing Ethernet MAC address,'):
                    with it('returns exactly one interface'):
                        intfs = self.api.list_interfaces(eth_mac_address=self.intf.config.protocols[0].eth.mac_address)
                        expect(intfs).to(have_len(1))
                        expect(intfs[0].id).to(equal(self.intf.id))

                with description('invalid Ethernet MAC address,'):
                    with it('returns no interfaces'):
                        intfs = self.api.list_interfaces(eth_mac_address='foo')
                        expect(intfs).to(be_empty)

                with description('missing Ethernet MAC address,'):
                    with it('returns no interfaces'):
                        intfs = self.api.list_interfaces(eth_mac_address='')
                        expect(intfs).to(be_empty)

                with description('known existing IPv4 address,'):
                    with it('returns exactly one interface'):
                        intfs = self.api.list_interfaces(ipv4_address=self.intf.config.protocols[1].ipv4.static.address)
                        expect(intfs).to(have_len(1))
                        expect(intfs[0].id).to(equal(self.intf.id))

                with description('invalid IPv4 address,'):
                    with it('returns no interfaces'):
                        intfs = self.api.list_interfaces(ipv4_address='foo')
                        expect(intfs).to(be_empty)

                with description('missing IPv4 address,'):
                    with it('returns no interfaces'):
                        intfs = self.api.list_interfaces(ipv4_address='')
                        expect(intfs).to(be_empty)

        with description('get,'):
            with description('known existing interface,'):
                with before.each:
                    intf = self.api.create_interface(example_interface(self.api.api_client))
                    expect(intf).to(be_valid_interface)
                    self.intf, self.cleanup = intf, intf

                with it('succeeds'):
                    expect(self.api.get_interface(self.intf.id)).to(be_valid_interface)

                with it('returns 405'):
                    expect(lambda: self.api.api_client.call_api('/interfaces', 'PUT')).to(raise_api_exception(405, headers={'Allow': "GET, POST"}))

            with description('non-existent interface,'):
                with it('returns 404'):
                    expect(lambda: self.api.get_interface('foo')).to(raise_api_exception(404))
            with description('invalid interface id,'):
                with it('returns 404'):
                    expect(lambda: self.api.get_interface('Invalid_interface')).to(raise_api_exception(404))

        with description('create,'):
            with before.each:
                self.intf = empty_interface(self.api.api_client)

            with description('Ethernet protocol,'):
                with before.each:
                    self.intf.config.protocols = map(as_interface_protocol, [
                        client.models.InterfaceProtocolConfigEth()
                    ])

                with description('broadcast MAC address'):
                    with before.each:
                        self.intf.config.protocols[0].eth.mac_address='ff:ff:ff:ff:ff:ff'

                    with it('returns 400'):
                        expect(lambda: self.api.create_interface(self.intf)).to(raise_api_exception(400))

                with description('multicast MAC address,'):
                    with before.each:
                        self.intf.config.protocols[0].eth.mac_address='01:00:00:00:00:01'

                    with it('returns 400'):
                        expect(lambda: self.api.create_interface(self.intf)).to(raise_api_exception(400))

                with description('too short MAC address,'):
                    with before.each:
                        self.intf.config.protocols[0].eth.mac_address='00:00:00:00:01'

                    with it('returns 400'):
                        expect(lambda: self.api.create_interface(self.intf)).to(raise_api_exception(400))

                with description('too long MAC address,'):
                    with before.each:
                        self.intf.config.protocols[0].eth.mac_address='00:00:00:00:00:00:01'

                    with it('returns 400'):
                        expect(lambda: self.api.create_interface(self.intf)).to(raise_api_exception(400))

                with description('garbage MAC address,'):
                    with before.each:
                        self.intf.config.protocols[0].eth.mac_address='00:00:00:00:00:fh'

                    with it('returns 400'):
                        expect(lambda: self.api.create_interface(self.intf)).to(raise_api_exception(400))

                with description('valid MAC address,'):
                    with before.each:
                        self.intf.config.protocols[0].eth.mac_address='00:00:00:00:00:01'

                    with it('succeeds'):
                        intf = self.api.create_interface(self.intf)
                        expect(intf).to(be_valid_interface)
                        self.cleanup = intf

            with description('IPv4 protocol,'):
                with before.each:
                    self.intf.config.protocols = map(as_interface_protocol, [
                        client.models.InterfaceProtocolConfigEth(mac_address='00:01:5c:a1:ab:1e'),
                        client.models.InterfaceProtocolConfigIpv4()
                    ])

                with description('static,'):
                    with before.each:
                        self.intf.config.protocols[1].ipv4.method='static'
                        self.intf.config.protocols[1].ipv4.static=(
                            client.models.InterfaceProtocolConfigIpv4Static())

                    with description('valid IPv4 address,'):
                        with before.each:
                            self.intf.config.protocols[1].ipv4.static.address='192.0.2.10'
                            self.intf.config.protocols[1].ipv4.static.prefix_length=24

                        with description('no gateway,'):
                            with it('succeeds'):
                                intf = self.api.create_interface(self.intf)
                                expect(intf).to(be_valid_interface)
                                self.cleanup = intf

                        with description('valid gateway,'):
                            with before.each:
                                self.intf.config.protocols[1].ipv4.static.gateway='192.0.2.1'

                            with it('succeeds'):
                                intf = self.api.create_interface(self.intf)
                                expect(intf).to(be_valid_interface)
                                self.cleanup = intf

                        with description('non-local gateway,'):
                            with before.each:
                                self.intf.config.protocols[1].ipv4.static.gateway='192.0.3.1'

                            with it('returns 400'):
                                expect(lambda: self.api.create_interface(self.intf)).to(raise_api_exception(400))

                        with description('too long prefix legnth,'):
                            with before.each:
                                self.intf.config.protocols[1].ipv4.static.prefix_length=34

                            with it('returns 400'):
                                expect(lambda: self.api.create_interface(self.intf)).to(raise_api_exception(400))

                        with description('invalid port,'):
                            with before.each:
                                self.intf.port_id = 4184

                            with it('returns 400'):
                                expect(lambda: self.api.create_interface(self.intf)).to(raise_api_exception(400))

                    with description('invalid IPv4 address,'):
                        with description('IPv6 address as IPv4 address,'):
                            with before.each:
                                self.intf.config.protocols[1].ipv4.static.address='2001:db8::10'
                                self.intf.config.protocols[1].ipv4.static.prefix_length=24

                            with it('returns 400'):
                                expect(lambda: self.api.create_interface(self.intf)).to(raise_api_exception(400))

                        with description('too short IPv4 address,'):
                            with before.each:
                                self.intf.config.protocols[1].ipv4.static.address='192.0.2'
                                self.intf.config.protocols[1].ipv4.static.prefix_length=24

                            with it('returns 400'):
                                expect(lambda: self.api.create_interface(self.intf)).to(raise_api_exception(400))

                        with description('too long IPv4 address,'):
                            with before.each:
                                self.intf.config.protocols[1].ipv4.static.address='192.0.2.10.10'
                                self.intf.config.protocols[1].ipv4.static.prefix_length=24

                            with it('returns 400'):
                                expect(lambda: self.api.create_interface(self.intf)).to(raise_api_exception(400))

                with description('dhcp,'):
                    with before.each:
                        self.intf.config.protocols[1].ipv4.method='dhcp'
                        self.intf.config.protocols[1].ipv4.dhcp=(
                            client.models.InterfaceProtocolConfigIpv4Dhcp())

                    with description('no properties,'):
                        with it('succeeds'):
                            intf = self.api.create_interface(self.intf)
                            expect(intf).to(be_valid_interface)
                            self.cleanup = intf

                    with description('hostname set,'):
                        with before.each:
                            self.intf.config.protocols[1].ipv4.dhcp.hostname='test-01'

                        with it('succeeds'):
                            intf = self.api.create_interface(self.intf)
                            expect(intf).to(be_valid_interface)
                            self.cleanup = intf

                        with description('client set,'):
                            with before.each:
                                self.intf.config.protocols[1].ipv4.dhcp.client='01.02.03.04.05.06'

                            with it('succeeds'):
                                intf = self.api.create_interface(self.intf)
                                expect(intf).to(be_valid_interface)
                                self.cleanup = intf

            with description('User-defined ID,'):
                with before.each:
                    self.intf.config.protocols = map(as_interface_protocol, [
                        client.models.InterfaceProtocolConfigEth()
                    ])

                with description('valid IDs,'):
                    with it('succeeds'):
                        self.intf.id = "interface-one-hundred"
                        self.intf.port_id = "0"
                        self.intf.config.protocols[0].eth.mac_address='00:00:00:00:00:01'
                        intf = self.api.create_interface(self.intf)
                        expect(intf).to(be_valid_interface)
                        expect(intf.id).to(equal("interface-one-hundred"))
                        expect(intf.port_id).to(equal("0"))
                        self.cleanup = intf

                with description('invalid interface ID,'):
                    with it('returns 400'):
                        self.intf.id = "Invalid_interface_id"
                        self.intf.port_id = "0"
                        self.intf.config.protocols[0].eth.mac_address='00:00:00:00:00:01'
                        print self.intf
                        expect(lambda: self.api.create_interface(self.intf)).to(raise_api_exception(400))

                with description('invalid port ID,'):
                    with it('returns 400'):
                        self.intf.id = "valid-intf-id"
                        self.intf.port_id = "Invalid_Port_id"
                        self.intf.config.protocols[0].eth.mac_address='00:00:00:00:00:01'
                        expect(lambda: self.api.create_interface(self.intf)).to(raise_api_exception(400))

        with description('update,'):
            with description('valid interface,'):
                with before.each:
                    intf = self.api.create_interface(example_interface(self.api.api_client))
                    expect(intf).to(be_valid_interface)
                    self.intf, self.cleanup = intf, intf

                with description('unsupported method,'):
                    with it('returns 405'):
                        expect(lambda: self.api.api_client.call_api('/interfaces/%s' % self.intf.id, 'PUT')).to(raise_api_exception(
                            405, headers={'Allow': "DELETE, GET"}))

            with description('non-existent interface,'):
                with description('unsupported method,'):
                    with it('returns 405'):
                        expect(lambda: self.api.api_client.call_api('/interfaces/foo', 'PUT')).to(raise_api_exception(
                            405, headers={'Allow': "DELETE, GET"}))

        with description('delete,'):
            with description('valid interface,'):
                with before.each:
                    intf = self.api.create_interface(example_interface(self.api.api_client))
                    expect(intf).to(be_valid_interface)
                    self.intf, self.cleanup = intf, intf

                with it('succeeds'):
                    self.api.delete_interface(self.intf.id)
                    expect(lambda: self.api.get_interface(self.intf.id)).to(raise_api_exception(404))
                    self.cleanup = None

            with description('non-existent interface,'):
                with it('succeeds'):
                    self.api.delete_interface('foo')

            with description('invalid interface ID,'):
                with it('returns 400'):
                    expect(lambda: self.api.delete_interface("Invalid_d")).to(raise_api_exception(404))

        with description('bulk-create,'):
            with description('IPv4 protocol,'):
                with description('static,'):
                    with before.each:
                        self.intfs = [ ipv4_interface(self.api.api_client, ipv4_address='192.0.2.{}'.format(i+1)) for i in range(BULK_OP_SIZE) ]

                    with description('valid interfaces,'):
                        with it('succeeds'):
                            resp = self.api.bulk_create_interfaces(client.models.BulkCreateInterfacesRequest(self.intfs))
                            expect(resp.items).to(have_len(len(self.intfs)))
                            for intf in resp.items:
                                expect(intf).to(be_valid_interface)
                            self.cleanup = resp.items

                    with description('single invalid interface,'):
                        with before.each:
                            self.intfs[-1].config.protocols[1].ipv4.static.gateway='10.0.0.1'

                        with it('returns 400 with no new interfaces'):
                            expect(lambda: self.api.bulk_create_interfaces(client.models.BulkCreateInterfacesRequest(self.intfs))).to(raise_api_exception(400))
                            intfs = self.api.list_interfaces()
                            expect(intfs).to(be_empty)

                with description('dhcp,'):
                    with before.each:
                        self.intfs = [ ipv4_interface(self.api.api_client) for i in range(BULK_OP_SIZE) ]

                    with it('succeeds'):
                        resp = self.api.bulk_create_interfaces(client.models.BulkCreateInterfacesRequest(self.intfs))
                        expect(resp.items).to(have_len(len(self.intfs)))
                        for intf in resp.items:
                            expect(intf).to(be_valid_interface)
                        self.cleanup = resp.items

            with description('unsupported method, '):
                with it('returns 405'):
                    expect(lambda: self.api.api_client.call_api('/interfaces/x/bulk-create', 'GET')).to(raise_api_exception(405, headers={'Allow': "POST"}))

        with description('bulk-delete,'):
            with before.each:
                self.intfs = [ ipv4_interface(self.api.api_client) for i in range(BULK_OP_SIZE) ]
                resp = self.api.bulk_create_interfaces(client.models.BulkCreateInterfacesRequest(self.intfs))
                expect(resp.items).to(have_len(len(self.intfs)))
                for intf in resp.items:
                    expect(intf).to(be_valid_interface)
                self.intfs, self.cleanup = resp.items, resp.items

            with description('delete one,'):
                with it('succeeds'):
                    self.api.bulk_delete_interfaces(client.models.BulkDeleteInterfacesRequest([self.intfs[-1].id]))
                    for intf in self.intfs[0:-1]:
                        expect(self.api.get_interface(intf.id)).to(be_valid_interface)
                    expect(lambda: self.api.get_interface(self.intfs[-1].id)).to(raise_api_exception(404))

            with description('delete all,'):
                with it('succeeds'):
                    ids = [ i.id for i in self.intfs ]
                    self.api.bulk_delete_interfaces(client.models.BulkDeleteInterfacesRequest(ids))
                    for x in ids:
                        expect(lambda: self.api.get_interface(x)).to(raise_api_exception(404))

            with description('delete existent and non-existent,'):
                with it('succeeds'):
                    self.api.bulk_delete_interfaces(client.models.BulkDeleteInterfacesRequest([self.intfs[0].id, 'foo']))
                    expect(lambda: self.api.get_interface(self.intfs[0].id)).to(raise_api_exception(404))
                    for x in [ i.id for i in self.intfs[1:] ]:
                        expect(self.api.get_interface(x)).to(be_valid_interface)

            with description('unsupported method, '):
                with it('returns 405'):
                    expect(lambda: self.api.api_client.call_api('/interfaces/x/bulk-delete', 'GET')).to(raise_api_exception(
                        405, headers={'Allow': "POST"}))

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

    with description ('configuration file,'):
        with before.all:
            service = Service(CONFIG.service('interface-config-file'))
            self.process = service.start()
            self.api = client.api.InterfacesApi(service.client())

        with it('verify interface created'):
            intfs = self.api.list_interfaces()
            expect(len(intfs)).to(equal(2))

            intfs = self.api.list_interfaces(port_id="port0")
            expect(len(intfs)).to(equal(1))
            expect(intfs[0].id).to(equal("interface-one"))
            expect(intfs[0].port_id).to(equal("port0"))

            intfs = self.api.list_interfaces(port_id="port1")
            expect(len(intfs)).to(equal(1))
            expect(intfs[0].id).to(equal("interface-two"))
            expect(intfs[0].port_id).to(equal("port1"))

        with after.all:
            try:
                self.process.terminate()
                self.process.wait()
            except AttributeError:
                pass
