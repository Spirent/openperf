import client.models
import re


# Default configurations
DURATION_CONFIG = {'frames': 100}
LOAD_CONFIG = {'rate': 1000}
TRAFFIC_CONFIG = [{'length': {'fixed': 128},
                   'packet': ['ethernet', 'ipv4', 'udp']}]


MAC_REGEX = '^([0-9A-Fa-f]{2}[:-]){5}([0-9A-Fa-f]{2})$'
IPV4_REGEX = '^([0-9]+.){3}[0-9]+$'
NUM_REGEX = '^[0-9]+&'


def get_modifier(thing):
    if type(thing) is int:
        return 'field', client.models.TrafficProtocolFieldModifier()
    elif re.match(MAC_REGEX, thing):
        return 'mac', client.models.TrafficProtocolMacModifier()
    elif re.match(IPV4_REGEX, thing):
        return 'ipv4', client.models.TrafficProtocolIpv4Modifier()
    else:
        return 'ipv6', client.models.TrafficProtocolIpv6Modifier()


def get_seq_modifier(thing):
    if type(thing) is int:
        return client.models.TrafficProtocolFieldModifierSequence()
    elif re.match(MAC_REGEX, thing):
        return client.models.TrafficProtocolMacModifierSequence()
    elif re.match(IPV4_REGEX, thing):
        return client.models.TrafficProtocolIpv4ModifierSequence()
    else:
        return client.models.TrafficProtocolIpv6ModifierSequence()


def make_traffic_modifiers(mod_config):
    modifiers = []

    for item in mod_config['items']:
        for name, config in item.items():
            if 'list' in config:
                prop, mod_impl = get_modifier(config['list'][0])
                mod_impl.list = config['list']
            else:
                seq_mod = get_seq_modifier(config['sequence']['start'])
                seq_mod.start = config['sequence']['start']
                seq_mod.count = config['sequence']['count']
                if 'stop' in config['sequence']:
                    seq_mod.stop = config['sequence']['stop']
                if 'skip' in config['sequence']:
                    seq_mod.skip = config['sequence']['skip']

                prop, mod_impl = get_modifier(config['sequence']['start'])
                mod_impl.sequence = seq_mod

        mod = client.models.TrafficProtocolModifier(name=name,
                                                    permute=False)
        if 'offset' in config:
            mod.offset = config['offset']

        setattr(mod, prop, mod_impl)

        modifiers.append(mod)

    tie = mod_config['tie'] if 'tie' in mod_config else 'zip'

    return client.models.TrafficProtocolModifiers(items=modifiers, tie=tie)



def make_traffic_template(packet_config):
    """Construct a traffic template from a simplified configuration model"""
    template = client.models.TrafficPacketTemplate()
    template.protocols = []

    for blob in packet_config:
        if type(blob) is str:
            blob = {blob: {}}

        if 'custom' in blob:
            custom = client.models.PacketProtocolCustom()
            for key, value in blob['custom'].items():
                setattr(custom, key, value)

            proto_custom = client.models.TrafficProtocol()
            proto_custom.custom = custom

            if 'modifiers' in blob['custom']:
                proto_custom.modifiers = make_traffic_modifiers(blob['custom']['modifiers'])

            template.protocols.append(proto_custom)

        if 'ethernet' in blob:
            eth = client.models.PacketProtocolEthernet()
            for key, value in blob['ethernet'].items():
                setattr(eth, key, value)

            proto_eth = client.models.TrafficProtocol()
            proto_eth.ethernet = eth

            if 'modifiers' in blob['ethernet']:
                proto_eth.modifiers = make_traffic_modifiers(blob['ethernet']['modifiers'])

            template.protocols.append(proto_eth)

        if 'mpls' in blob:
            mpls = client.models.PacketProtocolMpls()
            for key, value in blob['mpls'].items():
                setattr(mpls, key, value)

            proto_mpls = client.models.TrafficProtocol()
            proto_mpls.mpls = mpls

            if 'modifiers' in blob['mpls']:
                proto_mpls.modifiers = make_traffic_modifiers(blob['mpls']['modifiers'])

            template.protocols.append(proto_mpls)

        if 'vlan' in blob:
            vlan = client.models.PacketProtocolVlan()
            for key, value in blob['vlan'].items():
                setattr(vlan, key, value)

            proto_vlan = client.models.TrafficProtocol()
            proto_vlan.vlan = vlan

            if 'modifiers' in blob['vlan']:
                proto_vlan.modifiers = make_traffic_modifiers(blob['vlan']['modifiers'])

            template.protocols.append(proto_vlan)

        if 'ipv4' in blob:
            ipv4 = client.models.PacketProtocolIpv4()
            for key, value in blob['ipv4'].items():
                setattr(ipv4, key, value)

            proto_ipv4 = client.models.TrafficProtocol()
            proto_ipv4.ipv4 = ipv4

            if 'modifiers' in blob['ipv4']:
                proto_ipv4.modifiers = make_traffic_modifiers(blob['ipv4']['modifiers'])

            template.protocols.append(proto_ipv4)

        if 'ipv6' in blob:
            ipv6 = client.models.PacketProtocolIpv6()
            for key, value in blob['ipv6'].items():
                setattr(ipv6, key, value)

            proto_ipv6 = client.models.TrafficProtocol()
            proto_ipv6.ipv6 = ipv6

            if 'modifiers' in blob['ipv6']:
                proto_ipv6.modifiers = make_traffic_modifiers(blob['ipv6']['modifiers'])

            template.protocols.append(proto_ipv6)

        if 'tcp' in blob:
            tcp = client.models.PacketProtocolTcp()
            for key, value in blob['tcp'].items():
                setattr(tcp, key, value)

            proto_tcp = client.models.TrafficProtocol()
            proto_tcp.tcp = tcp

            if 'modifiers' in blob['tcp']:
                proto_tcp.modifiers = make_traffic_modifiers(blob['tcp']['modifiers'])

            template.protocols.append(proto_tcp)

        if 'udp' in blob:
            udp = client.models.PacketProtocolUdp()
            for key, value in blob['udp'].items():
                setattr(udp, key, value)

            proto_udp = client.models.TrafficProtocol()
            proto_udp.udp = udp

            if 'modifiers' in blob['udp']:
                proto_udp.modifiers = make_traffic_modifiers(blob['udp']['modifiers'])

            template.protocols.append(proto_udp)

    return template


def make_traffic_length(length_config):
    length = client.models.TrafficLength()

    if 'fixed' in length_config:
        length.fixed = length_config['fixed']
    elif 'list' in length_config:
        length.list = length_config['list']
    elif 'sequence' in length_config:
        sequence = client.models.TrafficLengthSequence()
        for key, value in length_config['sequence']:
            setattr(sequence, key, value)
        length.sequence = sequence

    return length


def make_traffic_signature(signature_config):
    if signature_config is None:
        return None

    signature = client.models.SpirentSignature()
    signature.stream_id = signature_config['stream_id']

    signature.latency = signature_config['latency'] if 'latency' in signature_config else 'end_of_frame'

    return signature


def make_traffic_definition(traffic_config):
    definition = client.models.TrafficDefinition()

    definition.packet = make_traffic_template(traffic_config['packet'])
    definition.length = make_traffic_length(traffic_config['length'])

    if 'signature' in traffic_config:
        definition.signature = make_traffic_signature(traffic_config['signature'])

    definition.weight = traffic_config['weight'] if 'weight' in traffic_config else 1

    if 'tie' in traffic_config:
        definition.packet.modifier_tie = traffic_config['tie']

    return definition


def make_traffic_duration(duration_config):
    duration = client.models.TrafficDuration()

    if 'continuous' in duration_config:
        duration.continuous = duration_config['continuous']
    elif 'frames' in duration_config:
        duration.frames = duration_config['frames']
    elif 'time' in duration_config:
        time = client.models.TrafficDurationTime()
        for key, value in duration_config['time'].items():
            setattr(time, key, value)
        duration.time = time

    return duration


def make_traffic_load(load_config):
    load = client.models.TrafficLoad()

    load.burst_size = load_config['burst_size'] if 'burst_size' in load_config else 1

    rate = client.models.TrafficLoadRate()
    if type(load_config['rate']) is int:
        rate.value = load_config['rate']
        rate.period = "seconds"
    else:
        for key, value in load_config['rate'].items():
            setattr(rate, key, value)

    load.rate = rate
    load.units = load_config['units'] if 'units' in load_config else 'frames'

    return load


def make_generator_config(**kwargs):
    duration_config = kwargs['duration'] if 'duration' in kwargs else DURATION_CONFIG
    load_config = kwargs['load'] if 'load' in kwargs else LOAD_CONFIG
    packet_config = kwargs['traffic'] if 'traffic' in kwargs else TRAFFIC_CONFIG

    config = client.models.PacketGeneratorConfig()
    config.duration = make_traffic_duration(duration_config)
    config.load = make_traffic_load(load_config)
    config.traffic = list(map(make_traffic_definition, packet_config))
    config.order = kwargs['order'] if 'order' in kwargs else 'round-robin'

    return config
