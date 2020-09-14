import client.api
import client.models
import time


def get_memory_dynamic_results_fields():
    fields = []
    swagger_types = client.models.MemoryGeneratorStats.swagger_types
    for (name, type) in swagger_types.items():
        if type in ['int', 'float']:
            fields.append('read.' + name)
            fields.append('write.' + name)
    return fields


def config_model():
    config = client.models.MemoryGeneratorConfig()
    config.buffer_size = 1024
    config.reads_per_sec = 128
    config.read_size = 8
    config.read_threads = 1
    config.writes_per_sec = 128
    config.write_size = 8
    config.write_threads = 1
    config.pattern = 'sequential'
    return config


def memory_generator_model(api_client, id = ''):
    gen = client.models.MemoryGenerator()
    gen.running = False
    gen.config = config_model()
    gen.id = id
    gen.init_percent_complete = 0
    return gen


def wait_for_buffer_initialization_done(api_client, generator_id, timeout):
    for i in range(timeout * 10):
        g7r = api_client.get_memory_generator(generator_id)
        if g7r.init_percent_complete == 100:
            return True
        time.sleep(.1)
    return False
