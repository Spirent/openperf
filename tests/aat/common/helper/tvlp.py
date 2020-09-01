import client.api
import client.models
import common.helper.block as block
import common.helper.memory as memory

def tvlp_model():
    tc = client.models.TvlpConfiguration()
    tc.profile = client.models.TvlpProfile()
    return tc

def tvlp_block_profile_model(entries, length, resource_id):
    pb = client.models.TvlpProfileBlock([])
    for i in range(entries):
        ps = client.models.TvlpProfileBlockSeries()
        ps.length = length
        ps.config = block.config_model()
        ps.resource_id = resource_id
        pb.series.append(ps)
    return pb

def tvlp_memory_profile_model(entries, length):
    pb = client.models.TvlpProfileMemory([])
    for i in range(entries):
        ps = client.models.TvlpProfileMemorySeries()
        ps.length = length
        ps.config = memory.config_model()
        pb.series.append(ps)
    return pb

def tvlp_cpu_profile_model(entries, length):
    pb = client.models.TvlpProfileCpu([])
    for i in range(entries):
        ps = client.models.TvlpProfileCpuSeries()
        ps.length = length
        ps.config = memory.config_model()
        pb.series.append(ps)
    return pb
