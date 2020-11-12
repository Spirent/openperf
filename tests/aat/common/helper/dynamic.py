import client.api
import client.models


def make_dynamic_results_config(fields):
    if not fields:
        return client.models.DynamicResultsConfig()

    thresholds = []
    digests = []

    conditions = ['equal', 'less', 'less_or_equal', 'greater', 'greater_or_equal']
    for condition in conditions:
        tconfig = client.models.ThresholdConfig()
        tconfig.id = ''
        tconfig.function = 'dx'
        tconfig.condition = condition
        tconfig.stat_x = fields[0]
        tconfig.value = 10
        thresholds.append(tconfig)

    for name in fields:
        tconfig = client.models.ThresholdConfig()
        tconfig.id = ''
        tconfig.function = 'dx'
        tconfig.condition = conditions[0]
        tconfig.value = 10
        tconfig.stat_x = name
        thresholds.append(tconfig)

    functions = ['dx', 'dxdy', 'dxdt']
    for function in functions:
        tconfig = client.models.ThresholdConfig()
        tconfig.id = ''
        tconfig.function = function
        tconfig.condition = conditions[0]
        tconfig.stat_x = fields[0]
        tconfig.stat_y = fields[1]
        tconfig.value = 10
        thresholds.append(tconfig)
        dconfig = client.models.TDigestConfig()
        dconfig.id = ''
        dconfig.function = function
        dconfig.stat_x = fields[0]
        dconfig.stat_y = fields[1]
        dconfig.compression = 10
        digests.append(dconfig)

    return client.models.DynamicResultsConfig(thresholds, digests)
