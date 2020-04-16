#include <zmq.h>
#include <sys/stat.h>
#include <poll.h>
#include <thread>

#include "core/op_core.h"
#include "packet/capture/api.hpp"
#include "packet/capture/sink.hpp"

#include "packet/capture/pcap_handler.hpp"
#include "packet/capture/pcap_io.hpp"
#include "packet/capture/capture_buffer.hpp"
#include "packet/capture/pistache_utils.hpp"

using namespace openperf::pistache_utils;

namespace openperf::packet::capture::api {

#if 0

// FIXME: This is just a dummy class for now
class CaptureToPcapReader
{
public:
    CaptureToPcapReader(const char* filename)
        : m_fp(nullptr)
        , m_length(0)
        , m_off(0)
    {
        struct stat st;
        if (stat(filename, &st) != 0) {
            throw std::runtime_error("Unable to open file");
        }
        m_fp = fopen(filename, "r");
        if (!m_fp) { throw std::runtime_error("Unable to open file"); }
        m_length = st.st_size;
    }

    size_t getLength() const { return m_length; }

    bool hasMore() const { return (m_off < m_length); }

    Pistache::RawBuffer getNextBuffer()
    {
        auto count = fread(m_data.data(), 1, m_data.size(), m_fp);
        m_off += count;
        return Pistache::RawBuffer(reinterpret_cast<char*>(m_data.data()),
                                   count);
    }

private:
    FILE* m_fp;
    size_t m_length;
    size_t m_off;
    std::array<uint8_t, 4096> m_data;
};


static Pistache::Async::Promise<ssize_t>
send_pcap_data(Pistache::Tcp::Transport* transport,
               int sockFd,
               std::shared_ptr<CaptureToPcapReader> reader,
               bool chunked)
{
    if (!reader->hasMore()) {
        if (chunked) {
            std::ostringstream os;
            os << std::hex << 0 << Pistache::Http::crlf << Pistache::Http::crlf;
            auto str = os.str();
            return transport->asyncWrite(
                sockFd, Pistache::RawBuffer(str, str.length()));
        }
        return Pistache::Async::Promise<ssize_t>::resolved(0);
    }

    auto buffer = reader->getNextBuffer();

    if (!chunked) {
        return transport->asyncWrite(sockFd, buffer)
            .then(
                [=](ssize_t) {
                    return send_pcap_data(transport, sockFd, reader, chunked);
                },
                Pistache::Async::Throw);
    } else {
        std::ostringstream os;
        os << std::hex << buffer.size() << Pistache::Http::crlf;
        std::string str = os.str();

        return transport
            ->asyncWrite(
                sockFd, Pistache::RawBuffer(str, str.length()), MSG_MORE)
            .then(
                [=](ssize_t) { return transport->asyncWrite(sockFd, buffer); },
                Pistache::Async::Throw)
            .then(
                [=](ssize_t) {
                    std::ostringstream os;
                    os << Pistache::Http::crlf;
                    auto crlf_str = os.str();

                    return transport->asyncWrite(
                        sockFd,
                        Pistache::RawBuffer(crlf_str, crlf_str.length()));
                },
                Pistache::Async::Throw)
            .then(
                [=](ssize_t) {
                    return send_pcap_data(transport, sockFd, reader, chunked);
                },
                Pistache::Async::Throw);
    }
}

Pistache::Async::Promise<ssize_t>
server_capture_pcap_async(Pistache::Http::ResponseWriter& response)
{
    auto mime_type =
        Pistache::Http::Mime::MediaType::fromString("application/x-pcapng");
    bool chunked = true;

    auto reader = std::make_shared<CaptureToPcapReader>(
        "/home/cmyers/proj/openperf/test.pcapng");

    // Update response headers
    auto& headers = response.headers();
    auto ct = headers.tryGet<Pistache::Http::Header::ContentType>();
    if (ct)
        ct->setMime(mime_type);
    else
        headers.add<Pistache::Http::Header::ContentType>(mime_type);
    if (chunked) {
        headers.add<Pistache::Http::Header::TransferEncoding>(
            Pistache::Http::Header::Encoding::Chunked);
    } else {
        headers.add<Pistache::Http::Header::ContentLength>(reader->getLength());
    }

    // Encode the headers into the buffer
    auto* buf = response.rdbuf();
    std::ostream os(buf);
    if (!write_status_line(os, response.version(), Pistache::Http::Code::Ok)
        || !write_cookies(os, response.cookies())
        || !write_headers(os, response.headers())
        || !(os << Pistache::Http::crlf)) {
        return Pistache::Async::Promise<ssize_t>::rejected(
            Pistache::Error("Response exceeded buffer size"));
    }

    auto* transport = get_tcp_transport(response);
    auto peer = response.peer();
    auto sockFd = peer->fd();

    return transport->asyncWrite(sockFd, buf->buffer(), MSG_MORE)
        .then(
            [=](ssize_t) {
                return send_pcap_data(transport, sockFd, reader, chunked);
            },
            Pistache::Async::Throw);
}

#endif

ssize_t
send_to_peer(Pistache::Tcp::Peer& peer, void* data, size_t len, int flags)
{
    int result;
#ifdef PISTACHE_USE_SSL
    if (peer->ssl() != NULL) {
        result = SSL_write((SSL*)peer.ssl(), data, len);
        if (result == 0) {
            auto error_code = SSL_get_error((SSL*)peer.ssl(), result);
            if (error_code != SSL_ERROR_WANT_WRITE) { result = -1; }
        }
    } else {
#endif /* PISTACHE_USE_SSL */
        result = ::send(peer.fd(), data, len, flags);
        if (result < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) { result = 0; }
        }
#ifdef PISTACHE_USE_SSL
    }
#endif /* PISTACHE_USE_SSL */
    return result;
}

ssize_t
sendn_to_peer(Pistache::Tcp::Peer& peer, void* data, size_t len, int flags)
{
    size_t total_sent = 0;
    while (total_sent < len) {
        auto remain = (len - total_sent);
        auto nsent = send_to_peer(
            peer, (void*)((uint8_t*)data + total_sent), remain, flags);
        if (nsent < 0) {
            // Unable to send
            break;
        }
        if (nsent == 0) {
            // Socket is full
            // FIXME:
            struct pollfd pfd
            {
                peer.fd(), POLLOUT, 0
            };
            int timeout = 2 * 1000; // 2 sec
            ::poll(&pfd, 1, timeout);
        }
        total_sent += nsent;
    }
    return total_sent;
}

Pistache::Async::Promise<ssize_t>
send_pcap_response_header(Pistache::Http::ResponseWriter& response,
                          uint32_t pcap_size)
{
    auto mime_type =
        Pistache::Http::Mime::MediaType::fromString("application/x-pcapng");

    // Update response headers
    auto& headers = response.headers();
    auto ct = headers.tryGet<Pistache::Http::Header::ContentType>();
    if (ct)
        ct->setMime(mime_type);
    else
        headers.add<Pistache::Http::Header::ContentType>(mime_type);
    if (pcap_size == 0) {
        headers.add<Pistache::Http::Header::TransferEncoding>(
            Pistache::Http::Header::Encoding::Chunked);
    } else {
        headers.add<Pistache::Http::Header::ContentLength>(pcap_size);
    }

// Encode the headers into the buffer
#if 0
    auto* buf = response.rdbuf();
    std::ostream os(buf);
#else
    std::ostringstream os;
#endif
    if (!write_status_line(os, response.version(), Pistache::Http::Code::Ok)
        || !write_cookies(os, response.cookies())
        || !write_headers(os, response.headers())
        || !(os << Pistache::Http::crlf)) {
        return Pistache::Async::Promise<ssize_t>::rejected(
            Pistache::Error("Response exceeded buffer size"));
    }

    auto peer = response.peer();

    // FIXME:
    // Need way to disable pistache threads from accessing/removing the peer

    // Send the HTTP response header
#if 0
    auto header_bytes_sent = sendn_to_peer(*peer,
                              (void*)buf->buffer().data().c_str(),
                              buf->buffer().size(),
                              MSG_MORE);
    if (header_bytes_sent != (ssize_t)buf->buffer().size()) {
        return Pistache::Async::Promise<ssize_t>::rejected(
            Pistache::Error("Unable to send HTTP response header"));
    }
#else
    auto str = os.str();
    auto header_bytes_sent =
        sendn_to_peer(*peer, (void*)str.c_str(), str.length(), MSG_MORE);
    if (header_bytes_sent != (ssize_t)str.length()) {
        return Pistache::Async::Promise<ssize_t>::rejected(
            Pistache::Error("Unable to send HTTP response header"));
    }
#endif
    return Pistache::Async::Promise<ssize_t>::resolved(header_bytes_sent);
}

class pcap_writer
{
public:
    pcap_writer(std::shared_ptr<Pistache::Tcp::Peer> peer)
        : m_peer(peer)
    {}

    size_t calc_length(reader& reader)
    {
        size_t header_length =
            pcapng::pad_block_length(sizeof(pcapng::section_block)
                                     + sizeof(uint32_t))
            + pcapng::pad_block_length(
                  sizeof(pcapng::interface_description_block) + sizeof(uint32_t)
                  + sizeof(pcapng::interface_default_options));
        size_t length = header_length;
        reader.rewind();
        reader.read([&](auto& buffer) {
            length += pcapng::pad_block_length(
                sizeof(pcapng::enhanced_packet_block) + sizeof(uint32_t)
                + buffer.captured_len);
            return true;
        });
        return length;
    }

    bool write_start(reader& reader)
    {
        reader.rewind();

        uint8_t* ptr = m_buffer.data();
        m_buffer_length = 0;
        m_buffer_sent = 0;
        m_error = false;

        {
            pcapng::section_block section;

            auto unpadded_block_length = sizeof(section) + sizeof(uint32_t);
            auto block_length = pcapng::pad_block_length(unpadded_block_length);

            section.block_type = pcapng::block_type::SECTION;
            section.block_total_length = block_length;
            section.byte_order_magic = pcapng::BYTE_ORDER_MAGIC;
            section.major_version = 1;
            section.minor_version = 0;
            section.section_length = pcapng::SECTION_LENGTH_UNSPECIFIED;
            std::copy(reinterpret_cast<uint8_t*>(&section),
                      reinterpret_cast<uint8_t*>(&section) + sizeof(section),
                      ptr + m_buffer_length);
            m_buffer_length += section.block_total_length;
            std::copy(reinterpret_cast<uint8_t*>(&section.block_total_length),
                      reinterpret_cast<uint8_t*>(&section.block_total_length)
                          + sizeof(section.block_total_length),
                      ptr + m_buffer_length
                          - sizeof(section.block_total_length));
        }

        {
            pcapng::interface_default_options interface_options;

            memset(&interface_options, 0, sizeof(interface_options));
            interface_options.ts_resol.hdr.option_code =
                pcapng::interface_option_type::IF_TSRESOL;
            interface_options.ts_resol.hdr.option_length = 1;
            interface_options.ts_resol.resolution = 9; // nano seconds
            interface_options.opt_end.hdr.option_code =
                pcapng::interface_option_type::OPT_END;
            interface_options.opt_end.hdr.option_length = 0;

            pcapng::interface_description_block interface_description;
            auto unpadded_block_length = sizeof(interface_description)
                                         + sizeof(uint32_t)
                                         + sizeof(interface_options);
            auto block_length = pcapng::pad_block_length(unpadded_block_length);

            interface_description.block_type =
                pcapng::block_type::INTERFACE_DESCRIPTION;
            interface_description.block_total_length = block_length;
            interface_description.link_type = pcapng::link_type::ETHERNET;
            interface_description.reserved = 0;
            interface_description.snap_len = 16384;

            std::copy(reinterpret_cast<uint8_t*>(&interface_description),
                      reinterpret_cast<uint8_t*>(&interface_description)
                          + sizeof(interface_description),
                      ptr + m_buffer_length);
            std::copy(reinterpret_cast<uint8_t*>(&interface_options),
                      reinterpret_cast<uint8_t*>(&interface_options)
                          + sizeof(interface_options),
                      ptr + m_buffer_length + sizeof(interface_description));
            m_buffer_length += interface_description.block_total_length;
            std::copy(reinterpret_cast<uint8_t*>(
                          &interface_description.block_total_length),
                      reinterpret_cast<uint8_t*>(
                          &interface_description.block_total_length)
                          + sizeof(interface_description.block_total_length),
                      ptr + m_buffer_length
                          - sizeof(interface_description.block_total_length));
        }
        assert(m_buffer_length < m_buffer.size());

        return flush();
    }

    bool write_packet(buffer_data& packet)
    {
        // Add packet block header
        pcapng::enhanced_packet_block block_hdr;
        block_hdr.block_type = pcapng::block_type::ENHANCED_PACKET;
        block_hdr.interface_id = 0;
        block_hdr.timestamp_high = packet.timestamp >> 32;
        block_hdr.timestamp_low = packet.timestamp;
        block_hdr.captured_len = packet.captured_len;
        block_hdr.packet_len = packet.packet_len;
        block_hdr.block_total_length = pcapng::pad_block_length(
            sizeof(block_hdr) + sizeof(uint32_t) + packet.captured_len);

        uint8_t* ptr = m_buffer.data();
        auto remain = m_buffer.size() - m_buffer_length;
        if (block_hdr.block_total_length > remain) {
            if (!flush()) { return false; }
            remain = m_buffer.size() - m_buffer_length;
        }
        if (block_hdr.block_total_length < remain) {
            std::copy(reinterpret_cast<uint8_t*>(&block_hdr),
                      reinterpret_cast<uint8_t*>(&block_hdr)
                          + sizeof(block_hdr),
                      ptr + m_buffer_length);
            std::copy(packet.data,
                      packet.data + packet.captured_len,
                      ptr + m_buffer_length + sizeof(block_hdr));
            m_buffer_length += block_hdr.block_total_length;
            std::copy(reinterpret_cast<uint8_t*>(&block_hdr.block_total_length),
                      reinterpret_cast<uint8_t*>(&block_hdr.block_total_length)
                          + sizeof(block_hdr.block_total_length),
                      ptr + m_buffer_length
                          - sizeof(block_hdr.block_total_length));
            return true;
        } else {
            // Packet is too big for buffer so just send directly
            return send_header_and_data(reinterpret_cast<uint8_t*>(&block_hdr),
                                        sizeof(block_hdr),
                                        packet.data,
                                        packet.captured_len);
        }
    }

    bool write_end(reader&) { return flush(); }

    bool flush()
    {
        if (!m_buffer_length) return true;

        auto remain = m_buffer_length - m_buffer_sent;
        if (remain) {
            auto n = sendn_to_peer(
                *m_peer, m_buffer.data() + m_buffer_sent, remain, 0);
            if (n < 0) {
                m_error = true;
                return false;
            }
            m_buffer_sent += n;
            m_total_bytes_sent += n;
        }
        if (m_buffer_sent == m_buffer_length) {
            m_buffer_length = 0;
            m_buffer_sent = 0;
            return true;
        }
        return false;
    }

    bool is_error() const { return m_error; }

    size_t get_total_bytes_sent() const { return m_total_bytes_sent; }
    void set_total_bytes_sent(size_t bytes) { m_total_bytes_sent = bytes; }

private:
    bool send_header_and_data(const uint8_t* hdr,
                              size_t hdr_len,
                              const uint8_t* data,
                              size_t data_len)
    {
        uint32_t total_block_len = pcapng::pad_block_length(hdr_len + data_len);
        auto pad_len = (total_block_len - hdr_len - data_len);
        if (sendn_to_peer(*m_peer, (void*)hdr, hdr_len, MSG_MORE)
            != (ssize_t)hdr_len) {
            m_error = true;
            return false;
        }
        m_total_bytes_sent += hdr_len;
        if (sendn_to_peer(*m_peer, (void*)data, data_len, 0)
            != (ssize_t)data_len) {
            m_error = true;
            return false;
        }
        m_total_bytes_sent += data_len;
        if (pad_len) {
            uint64_t pad = 0;
            if (sendn_to_peer(*m_peer, (void*)&pad, pad_len, MSG_MORE)
                != (ssize_t)pad_len) {
                m_error = true;
                return false;
            }
            m_total_bytes_sent += pad_len;
        }
        if (sendn_to_peer(
                *m_peer, (void*)&total_block_len, sizeof(total_block_len), 0)
            != (ssize_t)sizeof(total_block_len)) {
            m_error = true;
            return false;
        }
        m_total_bytes_sent += sizeof(total_block_len);
        return true;
    }

    std::shared_ptr<Pistache::Tcp::Peer> m_peer;
    std::array<uint8_t, 4096> m_buffer;
    size_t m_buffer_length = 0;
    size_t m_buffer_sent = 0;
    size_t m_total_bytes_sent = 0;
    bool m_error = false;
};

Pistache::Async::Promise<ssize_t>
server_capture_pcap_threaded(Pistache::Http::ResponseWriter& response,
                             std::shared_ptr<reader> capture_reader)
{
    auto capture_writer = std::make_unique<pcap_writer>(response.peer());
    auto result = send_pcap_response_header(
        response, capture_writer->calc_length(*capture_reader));
    if (result.isRejected()) return result;
    assert(result.isFulfilled());

    Pistache::Async::whenAny(result).then(
        [&](const Pistache::Async::Any& any) {
            // Include bytes from HTTP header in total count returned in promise
            capture_writer->set_total_bytes_sent(any.cast<ssize_t>());
        },
        Pistache::Async::NoExcept);

    struct transfer_context
    {
        std::unique_ptr<pcap_writer> writer;
        std::shared_ptr<reader> reader;
        Pistache::Async::Deferred<ssize_t> deferred;
        std::unique_ptr<std::thread> thread;
    };

    std::shared_ptr<transfer_context> context;

    auto promise = Pistache::Async::Promise<ssize_t>(
        [&](Pistache::Async::Deferred<ssize_t> deferred) mutable {
            context = std::make_shared<transfer_context>(
                transfer_context{std::move(capture_writer),
                                 capture_reader,
                                 std::move(deferred),
                                 nullptr});
        });

    // Send the Capture data from a thread
    context->thread = std::make_unique<std::thread>([context]() {
        context->writer->write_start(*context->reader);
        while (!context->reader->is_done() && !context->writer->is_error()) {
            context->reader->read([&context](auto& packet) {
                if (!context->writer->write_packet(packet)) return false;
                return true;
            });
        }
        if (context->writer->is_error()) {
            // Error writing data
            context->deferred.reject(
                Pistache::Error("Failed sending pcap data"));
            return;
        }
        context->writer->write_end(*context->reader);

        // FIXME: Need some way to enable the peer in pistache
        // thread again.
        //        This should be done through a message for
        //        thread safety
        context->deferred.resolve(
            (ssize_t)context->writer->get_total_bytes_sent());

        // FIXME: Currently the thread deletes itself so need to detach()
        context->thread->detach();
    });

    return promise;
}

Pistache::Async::Promise<ssize_t>
server_capture_pcap(Pistache::Http::ResponseWriter& response,
                    std::shared_ptr<reader> reader)
{
#if 1
    return server_capture_pcap_threaded(response, reader);
#else
    return server_capture_pcap_async(response, reader);
#endif
}

} // namespace openperf::packet::capture::api