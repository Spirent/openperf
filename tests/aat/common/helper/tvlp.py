import client.api
import client.models
import common.helper.block as block

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
