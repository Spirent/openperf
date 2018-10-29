from mamba import description, before, after, it
from expects import expect, be_empty
from common import Config, Service
from common.helper import ipv4_interface
from common.matcher import be_valid_interface, raise_api_exception

import client.api
import client.models
import os


CONFIG = Config(os.path.join(os.path.dirname(__file__), 'config.yaml'))
BULK_OP_SIZE = 4


with description('Interfaces,') as self:
    with before.all:
        service = Service(CONFIG.service())
        self.process = service.start()
        self.api = client.api.InterfacesApi(service.client())
        self.interfaces = list()

    with description('bulk-create,'):
        with description('static,'):
            with before.each:
                for i in range(BULK_OP_SIZE):
                    self.interfaces.append(
                        ipv4_interface(self.api.api_client,
                                       ipv4_address='192.0.2.{}'.format(i+1)))

            with description('valid interfaces,'):
                with it('succeeds'):
                    response = self.api.bulk_create_interfaces(
                        client.models.BulkCreateInterfacesRequest(self.interfaces))
                    expect(response.items).not_to(be_empty)
                    for item in response.items:
                        expect(item).to(be_valid_interface)

            with description('single invalid interface,'):
                with before.each:
                    self.interfaces[-1].config.protocols[1].ipv4.static.gateway='10.0.0.1'

                with it('returns 400 with no new interfaces'):
                    expect(lambda: self.api.bulk_create_interfaces(
                        client.models.BulkCreateInterfacesRequest(self.interfaces))).to(
                            raise_api_exception(400))
                    interfaces = self.api.list_interfaces()
                    expect(interfaces).to(be_empty)

        with description('dhcp,'):
            with before.each:
                for i in range(BULK_OP_SIZE):
                    self.interfaces.append(ipv4_interface(self.api.api_client))

            with it('succeeds'):
                response = self.api.bulk_create_interfaces(
                    client.models.BulkCreateInterfacesRequest(self.interfaces))
                expect(response.items).not_to(be_empty)
                for item in response.items:
                    expect(item).to(be_valid_interface)

    with description('bulk-delete,'):
        with before.each:
            self.created = list()
            for i in range(BULK_OP_SIZE):
                intf = self.api.create_interface(ipv4_interface(self.api.api_client))
                expect(intf).to(be_valid_interface)
                self.created.append(intf)

        with description('delete one,'):
            with it('succeeds'):
                self.api.bulk_delete_interfaces(
                    client.models.BulkDeleteInterfacesRequest([self.created[-1].id]))
                for x in [intf.id for intf in self.created[0:-1]]:
                    expect(self.api.get_interface(x)).to(be_valid_interface)
                expect(lambda: self.api.get_interface(self.created[-1].id)).to(raise_api_exception(404))

        with description('delete all,'):
            with it('succeeds'):
                ids = [intf.id for intf in self.created]
                self.api.bulk_delete_interfaces(
                    client.models.BulkDeleteInterfacesRequest(ids))
                for x in ids:
                    expect(lambda: self.api.get_interface(x)).to(raise_api_exception(404))

        with description('delete existent and non-existent,'):
            with it('succeeds'):
                self.api.bulk_delete_interfaces(
                    client.models.BulkDeleteInterfacesRequest([self.created[0].id,
                                                               'foo']))
                expect(lambda: self.api.get_interface(self.created[0].id)).to(raise_api_exception(404))
                for x in [intf.id for intf in self.created[1:]]:
                    expect(self.api.get_interface(x)).to(be_valid_interface)

    with after.each:
        del self.interfaces[:]
        for intf in self.api.list_interfaces():
            self.api.delete_interface(intf.id)

    with after.all:
        try:
            self.process.kill()
            self.process.wait()
        except AttributeError:
            pass
