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


def config_model(protocol = 'tcp'):
    config = client.models.NetworkGeneratorConfig()
    config.connections = 1
    config.ops_per_connection = 1
    config.reads_per_sec = 128
    config.read_size = 8
    config.writes_per_sec = 128
    config.write_size = 8
    config.target = client.models.NetworkGeneratorConfigTarget()
    config.target.host = "127.0.0.1"
    config.target.port = 3357
    config.target.protocol = protocol
    return config


def network_generator_model(api_client, id = ''):
    gen = client.models.NetworkGenerator()
    gen.running = False
    gen.config = config_model()
    gen.id = id
    return gen
