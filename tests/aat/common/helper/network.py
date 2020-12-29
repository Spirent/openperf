import client.api
import client.models
import time


def get_network_dynamic_results_fields():
    fields = []
    swagger_types = client.models.NetworkGeneratorStats.swagger_types
    for (name, type) in swagger_types.items():
        if type in ['int', 'float']:
            fields.append('read.' + name)
            fields.append('write.' + name)
    return fields

def config_model(protocol = 'tcp', host = '127.0.0.1', interface = 'lo'):
    config = client.models.NetworkGeneratorConfig()
    config.connections = 1
    config.ops_per_connection = 10
    config.reads_per_sec = 128
    config.read_size = 256
    config.writes_per_sec = 128
    config.write_size = 256
    config.target = client.models.NetworkGeneratorConfigTarget()
    config.target.host = host
    config.target.port = 3357
    config.target.protocol = protocol
    config.target.interface = interface
    return config


def network_generator_model(api_client, id = '', protocol = 'tcp', host = '127.0.0.1', interface = 'lo'):
    gen = client.models.NetworkGenerator()
    gen.running = False
    gen.config = config_model(protocol=protocol, host=host, interface=interface)
    gen.id = id
    return gen

def network_server_model(api_client, id = '', protocol = 'tcp', interface = 'lo'):
    gen = client.models.NetworkServer()
    gen.port = 3357
    gen.protocol = protocol
    gen.interface = interface
    gen.id = id
    return gen
