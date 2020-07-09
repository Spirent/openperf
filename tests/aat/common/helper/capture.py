import client.api
import shutil
import scapy.all

def make_capture_config(**kwargs):
    config = client.models.PacketCaptureConfig()

    if 'buffer_size' in kwargs:
        config.buffer_size = kwargs['buffer_size']
    if 'filter' in kwargs:
        config.filter = kwargs['filter']
    if 'start_trigger' in kwargs:
        config.start_trigger = kwargs['start_trigger']
    if 'stop_trigger' in kwargs:
        config.stop_trigger = kwargs['stop_trigger']
    return config

def get_capture_pcap(capture_api, result_id, pcap_file, packet_start=None, packet_end=None):
    kwargs = dict()
    # Need to use _preload_content to avoid issues with binary data
    kwargs['_preload_content'] = False
    if packet_start:
        kwargs['packet_start'] = packet_start
    if packet_end:
        kwargs['packet_end'] = packet_end

    resp = capture_api.get_packet_capture_pcap(id=result_id, **kwargs)
    with open(pcap_file, 'wb') as fdst:
        shutil.copyfileobj(resp, fdst)

def get_pcap_packet_count(pcap_file):
    count = 0
    for packet in scapy.all.rdpcap(pcap_file):
        count += 1
    return count
