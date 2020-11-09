import client.api
import client.models
import common.helper.block as block
import common.helper.memory as memory
import common.helper.cpu as cpu
import common.helper.packet as packet


def tvlp_model():
    tc = client.models.TvlpConfiguration()
    tc.profile = client.models.TvlpProfile()
    return tc


def tvlp_block_profile_model(entries, length, resource_id, time_scale=1.0, load_scale=1.0):
    tp = client.models.TvlpProfileBlock(series=[], time_scale=time_scale, load_scale=load_scale)
    for i in range(entries):
        ps = client.models.TvlpProfileBlockSeries()
        ps.length = length
        ps.config = block.config_model()
        ps.resource_id = resource_id
        tp.series.append(ps)
    return tp


def tvlp_memory_profile_model(entries, length, time_scale=1.0, load_scale=1.0):
    tp = client.models.TvlpProfileMemory(series=[], time_scale=time_scale, load_scale=load_scale)
    for i in range(entries):
        ps = client.models.TvlpProfileMemorySeries()
        ps.length = length
        ps.config = memory.config_model()
        tp.series.append(ps)
    return tp


def tvlp_cpu_profile_model(entries, length, time_scale=1.0, load_scale=1.0):
    tp = client.models.TvlpProfileCpu(series=[], time_scale=time_scale, load_scale=load_scale)
    for i in range(entries):
        ps = client.models.TvlpProfileCpuSeries()
        ps.length = length
        ps.config = cpu.config_model()
        tp.series.append(ps)
    return tp


def tvlp_packet_profile_model(entries, length, target_id, time_scale=1.0, load_scale=1.0):
    tp = client.models.TvlpProfilePacket(series=[], time_scale=time_scale, load_scale=load_scale)
    for i in range(entries):
        ps = client.models.TvlpProfilePacketSeries()
        ps.length = length
        ps.config = packet.config_model()
        ps.target_id = target_id
        tp.series.append(ps)
    return tp

def tvlp_profile_length(profile):
    length = 0.0
    for i in profile.series:
        length += profile.time_scale * i.length
    return length
