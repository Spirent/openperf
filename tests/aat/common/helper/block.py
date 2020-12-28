import client.api
import client.models
import time


def get_block_dynamic_results_fields():
    fields = []
    swagger_types = client.models.BlockGeneratorStats.swagger_types
    for (name, type) in swagger_types.items():
        if type in ['int', 'float']:
            fields.append('read.' + name)
            fields.append('write.' + name)
    return fields


def config_model():
    config = client.models.BlockGeneratorConfig()
    config.reads_per_sec = 1
    config.queue_depth = 1
    config.read_size = 1
    config.writes_per_sec = 1
    config.write_size = 1
    config.pattern = "random"
    return config


def block_generator_model(resource_id = None, id = '', running = False):
    bg = client.models.BlockGenerator()
    bg.id = id
    bg.resource_id = resource_id
    bg.running = running
    bg.config = config_model()
    return bg


def file_model(size = None, path = None, id = ''):
    ba = client.models.BlockFile()
    ba.id = id
    ba.size = size
    ba.path = path
    ba.init_percent_complete = 0
    ba.state = 'none'
    return ba


def wait_for_file_initialization_done(api_client, file_id, timeout):
    for i in range(timeout * 10):
        f = api_client.get_block_file(file_id)
        if f.init_percent_complete == 100:
            return True
        time.sleep(.1)
    return False
