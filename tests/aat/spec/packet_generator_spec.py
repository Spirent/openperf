from mamba import description, before, after
from expects import *
import os

import client.api
import client.models
from common import Config, Service
from common.helper import (make_traffic_template,
                           get_first_port_id,
                           get_second_port_id,
                           default_traffic_packet_template_with_seq_modifiers,
                           default_traffic_packet_template_with_list_modifiers,
                           packet_generator_model,
                           packet_generator_models,
                           wait_for_learning_resolved)
from common.matcher import (be_valid_packet_generator,
                            be_valid_packet_generator_result,
                            be_valid_transmit_flow,
                            raise_api_exception)
from common.helper import check_modules_exists
from common.helper import ipv4_interface


CONFIG = Config(os.path.join(os.path.dirname(__file__),
                             os.environ.get('MAMBA_CONFIG', 'config.yaml')))


CUSTOM_DATA = "TG9yZW0gaXBzdW0gZG9sb3Igc2l0IGFtZXQsIGNvbnNlY3RldHVyIGFkaXBpc2NpbmcgZWxpdCwg\
c2VkIGRvIGVpdXNtb2QgdGVtcG9yIGluY2lkaWR1bnQgdXQgbGFib3JlIGV0IGRvbG9yZSBtYWdu\
YSBhbGlxdWEuIFV0IGVuaW0gYWQgbWluaW0gdmVuaWFtLCBxdWlzIG5vc3RydWQgZXhlcmNpdGF0\
aW9uIHVsbGFtY28gbGFib3JpcyBuaXNpIHV0IGFsaXF1aXAgZXggZWEgY29tbW9kbyBjb25zZXF1\
YXQuIER1aXMgYXV0ZSBpcnVyZSBkb2xvciBpbiByZXByZWhlbmRlcml0IGluIHZvbHVwdGF0ZSB2\
ZWxpdCBlc3NlIGNpbGx1bSBkb2xvcmUgZXUgZnVnaWF0IG51bGxhIHBhcmlhdHVyLiBFeGNlcHRl\
dXIgc2ludCBvY2NhZWNhdCBjdXBpZGF0YXQgbm9uIHByb2lkZW50LCBzdW50IGluIGN1bHBhIHF1\
aSBvZmZpY2lhIGRlc2VydW50IG1vbGxpdCBhbmltIGlkIGVzdCBsYWJvcnVtLgo="

CUSTOM_L2_PACKET = [
    {'custom': {'data': CUSTOM_DATA,
                'layer': 'ethernet'}}
]

CUSTOM_PAYLOAD = [
    {'ethernet': {'source': '10:94:00:00:aa:bb',
                  'destination': '10:94:00:00:bb:cc'}},
    {'ipv4': {'source': '198.18.15.10',
              'destination': '198.18.15.20'}},
    'udp',
    {'custom': {'data': CUSTOM_DATA,
                'layer': 'payload'}}
]


with description('Packet Generator,', 'packet_generator') as self:
    with description('REST API'):

        with before.all:
            service = Service(CONFIG.service())
            self.process = service.start()
            self.api = client.api.PacketGeneratorsApi(service.client())
            self.intf_api = client.api.InterfacesApi(service.client())
            if not check_modules_exists(service.client(), 'packet-generator'):
                self.skip()

        with description('invalid HTTP methods,'):
            with description('/packet/generators,'):
                with it('returns 405'):
                    expect(lambda: self.api.api_client.call_api('/packet/generators', 'PUT')).to(
                        raise_api_exception(405, headers={'Allow': "DELETE, GET, POST"}))

            with description('/packet/generator-results,'):
                with it('returns 405'):
                    expect(lambda: self.api.api_client.call_api('/packet/generator-results', 'PUT')).to(
                        raise_api_exception(405, headers={'Allow': "DELETE, GET"}))

            with description('/packet/tx-flows,'):
                with it('returns 405'):
                    expect(lambda: self.api.api_client.call_api('/packet/tx-flows', 'PUT')).to(
                        raise_api_exception(405, headers={'Allow': "GET"}))

        with description('list generators,'):
            with before.each:
                gen = self.api.create_packet_generator(packet_generator_model(self.api.api_client))
                expect(gen).to(be_valid_packet_generator)
                self.generator = gen

            with description('unfiltered,'):
                with it('succeeds'):
                    generators = self.api.list_packet_generators()
                    expect(generators).not_to(be_empty)
                    for gen in generators:
                        expect(gen).to(be_valid_packet_generator)

            with description('filtered,'):
                with description('by target_id,'):
                    with it('returns an generator'):
                        generators = self.api.list_packet_generators(target_id=self.generator.target_id)
                        expect(generators).not_to(be_empty)
                        for ana in generators:
                            expect(ana).to(be_valid_packet_generator)
                        expect([ a for a in generators if a.id == self.generator.id ]).not_to(be_empty)

                with description('non-existent target_id,'):
                    with it('returns no generators'):
                        generators = self.api.list_packet_generators(target_id='foo')
                        expect(generators).to(be_empty)

        with description('get generator,'):
            with description('by existing generator id,'):
                with before.each:
                    gen = self.api.create_packet_generator(packet_generator_model(self.api.api_client))
                    expect(gen).to(be_valid_packet_generator)
                    self.generator = gen

                with it('succeeds'):
                    expect(self.api.get_packet_generator(self.generator.id)).to(be_valid_packet_generator)

            with description('non-existent generator,'):
                with it('returns 404'):
                    expect(lambda: self.api.get_packet_generator('foo')).to(raise_api_exception(404))

            with description('invalid generator id,'):
                with it('returns 404'):
                    expect(lambda: self.api.get_packet_generator(':bar:')).to(raise_api_exception(404))

        with description('create generator,'):
            with description('attached to port, '):
                with description('valid config,'):
                    with description('without modifiers,'):
                        with it('succeeds'):
                            gen = packet_generator_model(self.api.api_client)
                            result = self.api.create_packet_generator(gen)
                            expect(result).to(be_valid_packet_generator)

                    with description('with modifiers,'):
                        with description('with sequence modifiers'):
                            with it('succeeds'):
                                template = default_traffic_packet_template_with_seq_modifiers()
                                gen = packet_generator_model(self.api.api_client)
                                gen.config.traffic[0].packet = template
                                result = self.api.create_packet_generator(gen)
                                expect(result).to(be_valid_packet_generator)

                        with description('with permuted sequence modifiers,'):
                            with it('succeeds'):
                                template = default_traffic_packet_template_with_seq_modifiers(permute_flag=True)
                                gen = packet_generator_model(self.api.api_client)
                                gen.config.traffic[0].packet = template
                                result = self.api.create_packet_generator(gen)
                                expect(result).to(be_valid_packet_generator)

                        with description('with list modifiers'):
                            with it('succeeds'):
                                template = default_traffic_packet_template_with_list_modifiers()
                                gen = packet_generator_model(self.api.api_client)
                                gen.config.traffic[0].packet = template
                                result = self.api.create_packet_generator(gen)
                                expect(result).to(be_valid_packet_generator)

                        with description('with permuted list modifiers,'):
                            with it('succeeds'):
                                template = default_traffic_packet_template_with_list_modifiers(permute_flag=True)
                                gen = packet_generator_model(self.api.api_client)
                                gen.config.traffic[0].packet = template
                                result = self.api.create_packet_generator(gen)
                                expect(result).to(be_valid_packet_generator)

                    with description('with signatures enabled,'):
                        with it('succeeds'):
                            gen = packet_generator_model(self.api.api_client)
                            gen.config.traffic[0].signature = client.models.SpirentSignature(
                                stream_id=1, latency='start_of_frame')
                            result = self.api.create_packet_generator(gen)
                            expect(result).to(be_valid_packet_generator)

                    with description('with custom packet,'):
                        with it('succeeds'):
                            template = make_traffic_template(CUSTOM_L2_PACKET)
                            gen = packet_generator_model(self.api.api_client)
                            gen.config.traffic[0].packet = template
                            result = self.api.create_packet_generator(gen)
                            expect(result).to(be_valid_packet_generator)

                    with description('with custom payload,'):
                        with it('succeeds'):
                            template = make_traffic_template(CUSTOM_PAYLOAD)
                            gen = packet_generator_model(self.api.api_client)
                            gen.config.traffic[0].packet = template
                            result = self.api.create_packet_generator(gen)
                            expect(result).to(be_valid_packet_generator)

                with description('invalid config'):
                    with description('empty target id,'):
                        with it('returns 400'):
                            gen = packet_generator_model(self.api.api_client)
                            gen.target_id = None
                            expect(lambda: self.api.create_packet_generator(gen)).to(raise_api_exception(400))

                    with description('non-existent target id,'):
                        with it('returns 400'):
                            gen = packet_generator_model(self.api.api_client)
                            gen.target_id = 'foo'
                            expect(lambda: self.api.create_packet_generator(gen)).to(raise_api_exception(400))

                    with description('invalid ordering'):
                        with it('returns 400'):
                            gen = packet_generator_model(self.api.api_client)
                            gen.config.order = 'foo'
                            expect(lambda: self.api.create_packet_generator(gen)).to(raise_api_exception(400))

                    with description('invalid load,'):
                        with description('invalid schema,'):
                            with it('returns 400'):
                                gen = packet_generator_model(self.api.api_client)
                                gen.config.load.rate = -1
                                expect(lambda: self.api.create_packet_generator(gen)).to(raise_api_exception(400))

                    with description('invalid rate,'):
                        with it('returns 400'):
                            gen = packet_generator_model(self.api.api_client)
                            gen.config.load.rate.value = -1
                            expect(lambda: self.api.create_packet_generator(gen)).to(raise_api_exception(400))

                    with description('invalid duration,'):
                        with description('empty duration object,'):
                            with it('returns 400'):
                                gen = packet_generator_model(self.api.api_client)
                                gen.config.duration = client.models.TrafficDuration()
                                expect(lambda: self.api.create_packet_generator(gen)).to(raise_api_exception(400))

                        with description('negative frame count,'):
                            with it('returns 400'):
                                duration = client.models.TrafficDuration()
                                duration.frames = -1;
                                gen = packet_generator_model(self.api.api_client)
                                gen.config.duration = duration
                                expect(lambda: self.api.create_packet_generator(gen)).to(raise_api_exception(400))

                        with description('invalid time object,'):
                            with description('negative time,'):
                                with it('returns 400'):
                                    time = client.models.TrafficDurationTime()
                                    time.value = -1;
                                    time.units = "seconds"
                                    duration = client.models.TrafficDuration()
                                    duration.time = time
                                    gen = packet_generator_model(self.api.api_client)
                                    gen.config.duration = duration
                                    expect(lambda: self.api.create_packet_generator(gen)).to(raise_api_exception(400))

                            with description('bogus units,'):
                                with it('returns 400'):
                                    time = client.models.TrafficDurationTime()
                                    time.value = 10;
                                    time.units = "foobars"
                                    duration = client.models.TrafficDuration()
                                    duration.time = time
                                    gen = packet_generator_model(self.api.api_client)
                                    gen.config.duration = duration
                                    expect(lambda: self.api.create_packet_generator(gen)).to(raise_api_exception(400))

                    with description('invalid traffic definition,'):
                        with description('no traffic definition,'):
                            with it('returns 400'):
                                gen = packet_generator_model(self.api.api_client)
                                gen.config.traffic = []
                                expect(lambda: self.api.create_packet_generator(gen)).to(raise_api_exception(400))

                        with description('invalid packet,'):
                            with description('invalid modifier tie,'):
                                with it('returns 400'):
                                    gen = packet_generator_model(self.api.api_client)
                                    gen.config.traffic[0].packet.modifier_tie = 'foo'
                                    expect(lambda: self.api.create_packet_generator(gen)).to(raise_api_exception(400))

                            with description('invalid address,'):
                                with it('returns 400'):
                                    gen = packet_generator_model(self.api.api_client)
                                    gen.config.traffic[0].packet.protocols[0].ethernet.source = 'foo'
                                    expect(lambda: self.api.create_packet_generator(gen)).to(raise_api_exception(400))

                        with description('invalid length,'):
                            with description('invalid fixed length,'):
                                with it('returns 400'):
                                    length = client.models.TrafficLength()
                                    length.fixed = 16
                                    gen = packet_generator_model(self.api.api_client)
                                    gen.config.traffic[0].length = length
                                    expect(lambda: self.api.create_packet_generator(gen)).to(raise_api_exception(400))

                            with description('invalid list length,'):
                                with it('returns 400'):
                                    length = client.models.TrafficLength()
                                    length.list = [128, 256, 512, 0]
                                    gen = packet_generator_model(self.api.api_client)
                                    gen.config.traffic[0].length = length
                                    expect(lambda: self.api.create_packet_generator(gen)).to(raise_api_exception(400))

                            with description('invalid sequence length,'):
                                with description('invalid count,'):
                                    with it('returns 400'):
                                        seq = client.models.TrafficLengthSequence()
                                        seq.count = 0
                                        seq.start = 128
                                        length = client.models.TrafficLength()
                                        length.sequence = seq
                                        gen = packet_generator_model(self.api.api_client)
                                        gen.config.traffic[0].length = length
                                        expect(lambda: self.api.create_packet_generator(gen)).to(raise_api_exception(400))

                                with description('invalid start,'):
                                    with it('returns 400'):
                                        seq = client.models.TrafficLengthSequence()
                                        seq.count = 10
                                        seq.start = 0
                                        length = client.models.TrafficLength()
                                        length.sequence = seq
                                        gen = packet_generator_model(self.api.api_client)
                                        gen.config.traffic[0].length = length
                                        expect(lambda: self.api.create_packet_generator(gen)).to(raise_api_exception(400))

                            with description('invalid weight,'):
                                with it('returns 400'):
                                    gen = packet_generator_model(self.api.api_client)
                                    gen.config.traffic[0].weight = -1
                                    expect(lambda: self.api.create_packet_generator(gen)).to(raise_api_exception(400))

                        with description('invalid modifiers,'):
                            with description('too many flows,'):
                                with it('returns 400'):
                                    # total flows = 65536^3 which exceeds our flow limit of (1 << 48) - 1
                                    template = default_traffic_packet_template_with_seq_modifiers()
                                    template.protocols[0].modifiers.items[0].mac.sequence.count = 65536
                                    template.protocols[1].modifiers.items[0].ipv4.sequence.count = 65536
                                    template.protocols[1].modifiers.items[1].ipv4.sequence.count = 65536
                                    template.protocols[1].modifiers.tie = 'cartesian'
                                    template.modifier_tie = 'cartesian'
                                    gen = packet_generator_model(self.api.api_client)
                                    gen.config.traffic[0].packet = template
                                    expect(lambda: self.api.create_packet_generator(gen)).to(raise_api_exception(400))

                            with description('too many signature flows,'):
                                with it('returns 400'):
                                    # total flows = 256^2 which exceeds our signature flow limit of 64k - 1
                                    template = default_traffic_packet_template_with_seq_modifiers()
                                    template.protocols[1].modifiers.items[0].ipv4.sequence.count = 256
                                    template.protocols[1].modifiers.items[1].ipv4.sequence.count = 256
                                    template.protocols[1].modifiers.tie = 'cartesian'
                                    gen = packet_generator_model(self.api.api_client)
                                    gen.config.traffic[0].packet = template
                                    gen.config.traffic[0].signature = client.models.SpirentSignature(
                                        stream_id=1, latency='start_of_frame')
                                    expect(lambda: self.api.create_packet_generator(gen)).to(raise_api_exception(400))

            with description('attached to interface, '):
                with before.each:
                    self.intf1 = ipv4_interface(self.intf_api.api_client, method="static", ipv4_address="192.168.22.10", gateway="192.168.22.1", mac_address="aa:bb:cc:dd:ee:01")
                    self.intf1.target_id = get_first_port_id(self.api.api_client)
                    self.intf1.id = "interface-one"
                    intf1_impl = self.intf_api.create_interface(self.intf1)

                    self.target_ip = "192.168.22.1"
                    self.target_mac = "aa:bb:cc:dd:ee:20"
                    self.intf2 = ipv4_interface(self.intf_api.api_client, method="static", ipv4_address=self.target_ip, mac_address=self.target_mac)
                    self.intf2.port_id = get_second_port_id(self.api.api_client)
                    self.intf2.id = "interface-two"
                    intf2_impl = self.intf_api.create_interface(self.intf2)

                with description('valid config,'):
                    with description('without modifiers,'):
                        with it('succeeds'):
                            gen = packet_generator_model(self.api.api_client)
                            gen.target_id = "interface-one"
                            gen.id = "generator-one"
                            result = self.api.create_packet_generator(gen)
                            expect(result).to(be_valid_packet_generator)

                            # Wait for learning to resolve.
                            expect(wait_for_learning_resolved(self.api, "generator-one")).to(be_true)

                            # Verify MAC address resolved correctly.
                            learning_results = self.api.get_packet_generator_learning_results('generator-one')
                            expect(learning_results.resolved_state).to(equal('resolved'))

                            expected = client.models.PacketGeneratorLearningResultIpv4(ip_address=self.target_ip, mac_address=self.target_mac)
                            expect(learning_results.ipv4).to(contain(expected))

        with description('delete generator,'):
            with description('by existing generator id,'):
                with before.each:
                    gen = self.api.create_packet_generator(packet_generator_model(self.api.api_client))
                    expect(gen).to(be_valid_packet_generator)
                    self.generator = gen

                with it('succeeds'):
                    self.api.delete_packet_generator(self.generator.id)
                    expect(self.api.list_packet_generators()).to(be_empty)

            with description('non-existent generator id,'):
                with it('succeeds'):
                    self.api.delete_packet_generator('foo')

            with description('invalid generator id,'):
                with it('returns 404'):
                    expect(lambda: self.api.delete_packet_generator("invalid_id")).to(raise_api_exception(404))

        with description('start generator,'):
            with description('attached to port, '):
                with before.each:
                    gen = self.api.create_packet_generator(packet_generator_model(self.api.api_client))
                    expect(gen).to(be_valid_packet_generator)
                    self.generator = gen

                with description('by existing generator id,'):
                    with it('succeeds'):
                        result = self.api.start_packet_generator(self.generator.id)
                        expect(result).to(be_valid_packet_generator_result)

                with description('non-existent generator id,'):
                    with it('returns 404'):
                        expect(lambda: self.api.start_packet_generator('foo')).to(raise_api_exception(404))

            with description('attached to interface, '):
                with before.each:
                    self.intf1 = ipv4_interface(self.intf_api.api_client, method="static", ipv4_address="192.168.22.10", gateway="192.168.22.1", mac_address="aa:bb:cc:dd:ee:01")
                    self.intf1.target_id = get_first_port_id(self.api.api_client)
                    self.intf1.id = "interface-one"
                    intf1_impl = self.intf_api.create_interface(self.intf1)

                    self.intf2 = ipv4_interface(self.intf_api.api_client, method="static", ipv4_address="192.168.22.1", mac_address="aa:bb:cc:dd:ee:20")
                    self.intf2.port_id = get_second_port_id(self.api.api_client)
                    self.intf2.id = "interface-two"
                    intf2_impl = self.intf_api.create_interface(self.intf2)

                    gen = packet_generator_model(self.api.api_client)
                    gen.target_id = "interface-one"
                    gen.id = "generator-one"
                    result = self.api.create_packet_generator(gen)
                    expect(result).to(be_valid_packet_generator)

                    expect(wait_for_learning_resolved(self.api, "generator-one")).to(be_true)

                with description('by existing generator id,'):
                    with it('succeeds'):
                        result = self.api.start_packet_generator("generator-one")
                        expect(result).to(be_valid_packet_generator_result)

        with description('stop running generator,'):
            with before.each:
                gen = self.api.create_packet_generator(packet_generator_model(self.api.api_client))
                expect(gen).to(be_valid_packet_generator)
                self.generator = gen
                result = self.api.start_packet_generator(self.generator.id)
                expect(result).to(be_valid_packet_generator_result)

            with description('by generator id,'):
                with it('succeeds'):
                    gen = self.api.get_packet_generator(self.generator.id)
                    expect(gen).to(be_valid_packet_generator)
                    expect(gen.active).to(be_true)

                    self.api.stop_packet_generator(self.generator.id)

                    gen = self.api.get_packet_generator(self.generator.id)
                    expect(gen).to(be_valid_packet_generator)
                    expect(gen.active).to(be_false)

                    results = self.api.list_packet_generator_results(generator_id=self.generator.id)
                    expect(results).not_to(be_empty)
                    for result in results:
                        expect(result).to(be_valid_packet_generator_result)
                        expect(result.active).to(be_false)

            with description('restart generator, '):
                with it('succeeds'):
                    self.api.stop_packet_generator(self.generator.id)
                    result = self.api.start_packet_generator(self.generator.id)
                    expect(result).to(be_valid_packet_generator_result)
                    expect(result.active).to(be_true)

                    # We should now have two results: one active, one inactive
                    results = self.api.list_packet_generator_results(generator_id=self.generator.id)
                    expect(results).not_to(be_empty)
                    for result in results:
                        expect(result).to(be_valid_packet_generator_result)
                    expect([r for r in results if r.active is True]).not_to(be_empty)
                    expect([r for r in results if r.active is False]).not_to(be_empty)

        with description('toggle generators,'):
            with before.each:
                gen = self.api.create_packet_generator(packet_generator_model(self.api.api_client))
                expect(gen).to(be_valid_packet_generator)
                self.generator = gen
                result = self.api.start_packet_generator(self.generator.id)
                expect(result).to(be_valid_packet_generator_result)
                expect(result.active).to(be_true)
                self.result = result

            with description('two valid generators,'):
                with it('succeeds'):
                    newgen = self.api.create_packet_generator(packet_generator_model(self.api.api_client))
                    expect(newgen).to(be_valid_packet_generator)
                    expect(newgen.id).not_to(equal(self.generator.id))

                    toggle = client.models.TogglePacketGeneratorsRequest()
                    toggle.replace = self.generator.id
                    toggle._with = newgen.id

                    result1 = self.api.toggle_packet_generators(toggle)
                    expect(result1).to(be_valid_packet_generator_result)
                    expect(result1.active).to(be_true)

                    result2 = self.api.get_packet_generator_result(self.result.id)
                    expect(result2).to(be_valid_packet_generator_result)
                    expect(result2.active).to(be_false)

            with description('non-existent generator,'):
                with it('returns 400'):
                    toggle = client.models.TogglePacketGeneratorsRequest()
                    toggle.replace = self.generator.id
                    toggle._with = 'foo'
                    expect(lambda: self.api.toggle_packet_generators(toggle)).to(raise_api_exception(404))

        with description('list generator results,'):
            with before.each:
                gen = self.api.create_packet_generator(packet_generator_model(self.api.api_client))
                expect(gen).to(be_valid_packet_generator)
                self.generator = gen
                result = self.api.start_packet_generator(self.generator.id)
                expect(result).to(be_valid_packet_generator_result)

            with description('unfiltered,'):
                with it('succeeds'):
                    results = self.api.list_packet_generator_results()
                    expect(results).not_to(be_empty)
                    for result in results:
                        expect(result).to(be_valid_packet_generator_result)

            with description('by generator id,'):
                with it('succeeds'):
                    results = self.api.list_packet_generator_results(generator_id=self.generator.id)
                    for result in results:
                        expect(result).to(be_valid_packet_generator_result)
                    expect([ r for r in results if r.generator_id == self.generator.id ]).not_to(be_empty)

            with description('non-existent generator id,'):
                with it('returns no results'):
                    results = self.api.list_packet_generator_results(generator_id='foo')
                    expect(results).to(be_empty)

            with description('by target id,'):
                with it('succeeds'):
                    results = self.api.list_packet_generator_results(target_id=get_first_port_id(self.api.api_client))
                    expect(results).not_to(be_empty)

            with description('non-existent target id,'):
                with it('returns no results'):
                    results = self.api.list_packet_generator_results(target_id='bar')
                    expect(results).to(be_empty)

        with description('list tx flows,'):
            with before.each:
                gen = self.api.create_packet_generator(packet_generator_model(self.api.api_client))
                expect(gen).to(be_valid_packet_generator)
                self.generator = gen
                result = self.api.start_packet_generator(self.generator.id)
                expect(result).to(be_valid_packet_generator_result)
                self.result = result

            with description('unfiltered,'):
                with it('succeeds'):
                    flows = self.api.list_tx_flows()
                    expect(flows).not_to(be_empty)
                    for flow in flows:
                        expect(flow).to(be_valid_transmit_flow)

            with description('filtered,'):
                with description('by target_id,'):
                    with it('returns tx flows'):
                        flows = self.api.list_tx_flows(target_id=self.generator.target_id)
                        expect(flows).not_to(be_empty)
                        for flow in flows:
                            expect(flow).to(be_valid_transmit_flow)
                        expect([f for f in flows if flow.generator_result_id == self.result.id]).not_to(be_empty)

                with description('non-existent target_id,'):
                    with it('returns no flows'):
                        flows = self.api.list_packet_generators(target_id='foo')
                        expect(flows).to(be_empty)

                with description('by generator_id,'):
                    with it('returns tx flows'):
                        flows = self.api.list_tx_flows(generator_id=self.generator.id)
                        expect(flows).not_to(be_empty)
                        for flow in flows:
                            expect(flow).to(be_valid_transmit_flow)
                            # Get generator result of tx flow
                            result = self.api.get_packet_generator_result(id=flow.generator_result_id)
                            expect(result).to(be_valid_packet_generator_result)
                            # Result generator id should match self generator id
                            expect(result.generator_id == self.generator.id).to(be_true)

                with description('non-existent generator_id,'):
                    with it('returns no flows'):
                        flows = self.api.list_packet_generators(target_id='bar')
                        expect(flows).to(be_empty)

        with description('get tx flow,'):
            with before.each:
                gen = self.api.create_packet_generator(packet_generator_model(self.api.api_client))
                expect(gen).to(be_valid_packet_generator)
                self.generator = gen
                result = self.api.start_packet_generator(self.generator.id)
                expect(result).to(be_valid_packet_generator_result)
                self.result = result

            with description('by flow id,'):
                with it('returns tx flow'):
                    result = self.api.get_packet_generator_result(self.result.id)
                    for flow_id in result.flows:
                        flow = self.api.get_tx_flow(flow_id)
                        expect(flow).to(be_valid_transmit_flow)

            with description('non-existent id,'):
                with it('returns 404'):
                    expect(lambda: self.api.get_tx_flow('foo')).to(raise_api_exception(404))

            with description('invalid generator id,'):
                with it('returns 404'):
                    expect(lambda: self.api.get_packet_generator(':bar:')).to(raise_api_exception(404))

        with description('bulk operations,'):
            with description('bulk create,'):
                with description('valid request,'):
                    with it('succeeds'):
                        request = client.models.BulkCreatePacketGeneratorsRequest()
                        request.items = packet_generator_models(self.api.api_client)
                        reply = self.api.bulk_create_packet_generators(request)
                        expect(reply.items).to(have_len(len(request.items)))
                        for item in reply.items:
                            expect(item).to(be_valid_packet_generator)

                with description('invalid requests,'):
                    with it('returns 400 for invalid config'):
                        request = client.models.BulkCreatePacketGeneratorsRequest()
                        request.items = packet_generator_models(self.api.api_client)
                        request.items[-1].config.load.rate.value = -1
                        expect(lambda: self.api.bulk_create_packet_generators(request)).to(raise_api_exception(400))
                        expect(self.api.list_packet_generators()).to(be_empty)

                    with it('returns 404 for invalid id'):
                        request = client.models.BulkCreatePacketGeneratorsRequest()
                        request.items = packet_generator_models(self.api.api_client)
                        request.items[-1].id = ':foo'
                        expect(lambda: self.api.bulk_create_packet_generators(request)).to(raise_api_exception(404))
                        expect(self.api.list_packet_generators()).to(be_empty)

            with description('bulk delete,'):
                with before.each:
                    request = client.models.BulkCreatePacketGeneratorsRequest()
                    request.items = packet_generator_models(self.api.api_client)
                    reply = self.api.bulk_create_packet_generators(request)
                    expect(reply.items).to(have_len(len(request.items)))
                    for item in reply.items:
                        expect(item).to(be_valid_packet_generator)

                with description('valid request,'):
                    with it('succeeds'):
                        self.api.bulk_delete_packet_generators(
                            client.models.BulkDeletePacketGeneratorsRequest(
                            [gen.id for gen in self.api.list_packet_generators()]))
                        expect(self.api.list_packet_generators()).to(be_empty)

                with description('invalid requests,'):
                    with it('succeeds with a non-existent id'):
                        self.api.bulk_delete_packet_generators(
                            client.models.BulkDeletePacketGeneratorsRequest(
                                [gen.id for gen in self.api.list_packet_generators()] + ['foo']))
                        expect(self.api.list_packet_generators()).to(be_empty)

                    with it('returns 404 for an invalid id'):
                        expect(lambda: self.api.bulk_delete_packet_generators(
                            client.models.BulkDeletePacketGeneratorsRequest(
                                [gen.id for gen in self.api.list_packet_generators()] + [':bar']))).to(
                                    raise_api_exception(404))
                        expect(self.api.list_packet_generators()).not_to(be_empty)

            with description('bulk start,'):
                with before.each:
                    request = client.models.BulkCreatePacketGeneratorsRequest()
                    request.items = packet_generator_models(self.api.api_client)
                    reply = self.api.bulk_create_packet_generators(request)
                    expect(reply.items).to(have_len(len(request.items)))
                    for item in reply.items:
                        expect(item).to(be_valid_packet_generator)

                with description('valid request,'):
                    with it('succeeds'):
                        reply = self.api.bulk_start_packet_generators(
                            client.models.BulkStartPacketGeneratorsRequest(
                            [gen.id for gen in self.api.list_packet_generators()]))
                        expect(reply.items).to(have_len(len(self.api.list_packet_generators())))
                        for item in reply.items:
                            expect(item).to(be_valid_packet_generator_result)
                            expect(item.active).to(be_true)

                with description('invalid requests,'):
                    with it('returns 404 for non-existent id'):
                        expect(lambda: self.api.bulk_start_packet_generators(
                            client.models.BulkStartPacketGeneratorsRequest(
                                [ana.id for ana in self.api.list_packet_generators()] + ['foo']))).to(
                                    raise_api_exception(404))
                        for ana in self.api.list_packet_generators():
                            expect(ana.active).to(be_false)

                    with it('returns 404 for invalid id'):
                        expect(lambda: self.api.bulk_start_packet_generators(
                            client.models.BulkStartPacketGeneratorsRequest(
                                [ana.id for ana in self.api.list_packet_generators()] + [':bar']))).to(
                                    raise_api_exception(404))
                        for ana in self.api.list_packet_generators():
                            expect(ana.active).to(be_false)

            with description('bulk stop,'):
                with before.each:
                    create_request = client.models.BulkCreatePacketGeneratorsRequest()
                    create_request.items = packet_generator_models(self.api.api_client)
                    create_reply = self.api.bulk_create_packet_generators(create_request)
                    expect(create_reply.items).to(have_len(len(create_request.items)))
                    for item in create_reply.items:
                        expect(item).to(be_valid_packet_generator)
                    start_reply = self.api.bulk_start_packet_generators(
                        client.models.BulkStartPacketGeneratorsRequest(
                            [gen.id for gen in create_reply.items]))
                    expect(start_reply.items).to(have_len(len(create_request.items)))
                    for item in start_reply.items:
                        expect(item).to(be_valid_packet_generator_result)
                        expect(item.active).to(be_true)

                with description('valid request,'):
                    with it('succeeds'):
                        self.api.bulk_stop_packet_generators(
                            client.models.BulkStopPacketGeneratorsRequest(
                                [gen.id for gen in self.api.list_packet_generators()]))
                        generators = self.api.list_packet_generators()
                        expect(generators).not_to(be_empty)
                        for gen in generators:
                            expect(gen.active).to(be_false)

                with description('invalid requests,'):
                    with it('succeeds with a non-existent id'):
                        self.api.bulk_stop_packet_generators(
                            client.models.BulkStopPacketGeneratorsRequest(
                                [gen.id for gen in self.api.list_packet_generators()] + ['foo']))
                        generators = self.api.list_packet_generators()
                        expect(generators).not_to(be_empty)
                        for gen in generators:
                            expect(gen.active).to(be_false)

                    with it('returns 404 for an invalid id'):
                        expect(lambda: self.api.bulk_stop_packet_generators(
                            client.models.BulkStopPacketGeneratorsRequest(
                                [gen.id for gen in self.api.list_packet_generators()] + [':bar']))).to(
                                   raise_api_exception(404))
                        generators = self.api.list_packet_generators()
                        expect(generators).not_to(be_empty)
                        for gen in generators:
                            expect(gen.active).to(be_true)

        with after.each:
            try:
                for gen in self.api.list_packet_generators():
                    if gen.active:
                        self.api.stop_packet_generator(gen.id)
                self.api.delete_packet_generators()

                for intf in self.intf_api.list_interfaces():
                    self.intf_api.delete_interface(intf.id)
            except AttributeError:
                pass
            self.generator = None
            self.result = None

        with after.all:
            try:
                self.process.terminate()
                self.process.wait()
            except AttributeError:
                pass
