import client.api
import client.models
import common.helper.block as block
import common.helper.memory as memory
import common.helper.cpu as cpu
import common.helper.packet as packet
import common.helper.network as network
import common.helper.dynamic as dynamic


def tvlp_model():
    tc = client.models.TvlpConfiguration()
    tc.profile = client.models.TvlpProfile()
    return tc


def tvlp_block_profile_model(entries, length, resource_id):
    tp = client.models.TvlpProfileBlock(series=[])
    for i in range(entries):
        ps = client.models.TvlpProfileBlockSeries()
        ps.length = length
        ps.config = block.config_model()
        ps.resource_id = resource_id
        tp.series.append(ps)
    return tp


def tvlp_memory_profile_model(entries, length):
    tp = client.models.TvlpProfileMemory(series=[])
    for i in range(entries):
        ps = client.models.TvlpProfileMemorySeries()
        ps.length = length
        ps.config = memory.config_model()
        tp.series.append(ps)
    return tp


def tvlp_cpu_profile_model(entries, length):
    tp = client.models.TvlpProfileCpu(series=[])
    for i in range(entries):
        ps = client.models.TvlpProfileCpuSeries()
        ps.length = length
        ps.config = cpu.config_model()
        tp.series.append(ps)
    return tp


def tvlp_packet_profile_model(entries, length, target_id):
    tp = client.models.TvlpProfilePacket(series=[])
    for i in range(entries):
        ps = client.models.TvlpProfilePacketSeries()
        ps.length = length
        ps.config = packet.config_model()
        ps.target_id = target_id
        tp.series.append(ps)
    return tp


def tvlp_network_profile_model(entries, length):
    tp = client.models.TvlpProfileNetwork(series=[])
    for i in range(entries):
        ps = client.models.TvlpProfileNetworkSeries()
        ps.length = length
        ps.config = network.config_model('udp')
        tp.series.append(ps)
    return tp


def tvlp_start_configuration(time_scale=1.0, load_scale=1.0):
    start = client.models.TvlpStartConfiguration()
    start.memory = client.models.TvlpStartSeriesConfiguration()
    start.memory.load_scale = load_scale
    start.memory.time_scale = time_scale
    start.memory.dynamic_results = dynamic.make_dynamic_results_config(
        memory.get_memory_dynamic_results_fields())
    start.block = client.models.TvlpStartSeriesConfiguration()
    start.block.load_scale = load_scale
    start.block.time_scale = time_scale
    start.block.dynamic_results = dynamic.make_dynamic_results_config(
        block.get_block_dynamic_results_fields())
    start.cpu = client.models.TvlpStartSeriesConfiguration()
    start.cpu.load_scale = load_scale
    start.cpu.time_scale = time_scale
    start.cpu.dynamic_results = dynamic.make_dynamic_results_config(
        ["utilization", "steal"])
    start.network = client.models.TvlpStartSeriesConfiguration()
    start.network.load_scale = load_scale
    start.network.time_scale = time_scale
    start.network.dynamic_results = dynamic.make_dynamic_results_config(
        network.get_network_dynamic_results_fields())
    return start


def tvlp_profile_length(profile):
    length = 0.0
    for i in profile.series:
        length += profile.time_scale * i.length
    return length
