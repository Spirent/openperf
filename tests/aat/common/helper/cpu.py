import client.api
import client.models


def get_cpu_dynamic_results_fields(generator_config):
    fields = []
    for (name, type) in client.models.CpuGeneratorStats.swagger_types.items():
        if type in ['int', 'float']:
            fields.append(name)

    for i in range(len(generator_config.cores)):
        swagger_types = client.models.CpuGeneratorCoreStats.swagger_types
        for (name, type) in swagger_types.items():
            if type in ['int', 'float']:
                fields.append('cores[' + str(i) + '].' + name)

    return fields

def config_model():
    core_config_target = client.models.CpuGeneratorCoreConfigTargets()
    core_config_target.instruction_set = 'scalar'
    core_config_target.data_type = 'float32';
    core_config_target.weight = 1

    core_config = client.models.CpuGeneratorCoreConfig()
    core_config.utilization = 50.0
    core_config.targets = [core_config_target]

    config = client.models.CpuGeneratorConfig()
    config.cores = [core_config]

    return config

def cpu_generator_model(api_client, running = False, id = ''):
    gen = client.models.CpuGenerator()
    gen.running = running
    gen.config = config_model()
    gen.id = id
    return gen
