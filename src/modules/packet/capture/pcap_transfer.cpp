#include <zmq.h>
#include <sys/stat.h>
#include <thread>

#include "core/op_core.h"
#include "packet/capture/api.hpp"
#include "packet/capture/sink.hpp"
#include "packet/capture/capture_buffer.hpp"

#include "packet/capture/pcap_defs.hpp"
#include "packet/capture/pcap_writer.hpp"
#include "packet/capture/pcap_transfer.hpp"

#include "packet/capture/pistache_utils.hpp"

using namespace openperf::pistache_utils;

namespace openperf::packet::capture::pcap {

const std::string PCAPNG_MIME_TYPE = "application/x-pcapng";

size_t calc_pcap_file_length(capture_buffer_reader& reader)
{
    size_t length = pcap_buffer_writer::traits::file_header_length()
                    + pcap_buffer_writer::traits::file_trailer_length();

    reader.rewind();
    for (auto& packet : reader) {
        length +=
            pcap_buffer_writer::traits::packet_length(packet.hdr.captured_len);
    };
    reader.rewind();

    return length;
}

class pcap_transfer_context : public transfer_context
{
public:
    virtual ~pcap_transfer_context() = default;

    void set_reader(std::unique_ptr<capture_buffer_reader>& reader) override
    {
        m_reader.reset(reader.release());
    }

    bool is_done() const override { return m_done; }

    void set_done(bool done) { m_done = done; }

    virtual Pistache::Async::Promise<ssize_t> start() = 0;

    virtual size_t get_total_length() const = 0;

protected:
    std::unique_ptr<capture_buffer_reader> m_reader;
    Pistache::Async::Deferred<ssize_t> m_deferred;
    bool m_done = false;
};

Pistache::Async::Promise<ssize_t>
send_pcap_response_header_async(Pistache::Http::ResponseWriter& response,
                                uint32_t pcap_size)
{
    auto mime_type =
        Pistache::Http::Mime::MediaType::fromString(PCAPNG_MIME_TYPE);

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
    auto* buf = response.rdbuf();
    std::ostream os(buf);
    if (!write_status_line(os, response.version(), Pistache::Http::Code::Ok)
        || !write_cookies(os, response.cookies())
        || !write_headers(os, response.headers())
        || !(os << Pistache::Http::crlf)) {
        return Pistache::Async::Promise<ssize_t>::rejected(
            Pistache::Error("Response exceeded buffer size"));
    }

    auto transport = get_transport(response);
    auto peer = response.peer();

    return transport->asyncWrite(peer->fd(), buf->buffer(), MSG_MORE);
}

Pistache::Async::Promise<ssize_t>
send_pcap_response_header_async(Pistache::Http::ResponseWriter& response,
                                transfer_context& context)
{
    return send_pcap_response_header_async(
        response,
        dynamic_cast<pcap_transfer_context&>(context).get_total_length());
}

Pistache::Async::Promise<ssize_t>
send_pcap_response_header(Pistache::Http::ResponseWriter& response,
                          uint32_t pcap_size)
{
    auto mime_type =
        Pistache::Http::Mime::MediaType::fromString(PCAPNG_MIME_TYPE);

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
    std::ostringstream os;

    if (!write_status_line(os, response.version(), Pistache::Http::Code::Ok)
        || !write_cookies(os, response.cookies())
        || !write_headers(os, response.headers())
        || !(os << Pistache::Http::crlf)) {
        return Pistache::Async::Promise<ssize_t>::rejected(
            Pistache::Error("Response exceeded buffer size"));
    }

    auto peer = response.peer();

    // Send the HTTP response header
    auto str = os.str();
    auto header_bytes_sent =
        send_to_peer_timeout(*peer, (void*)str.c_str(), str.length(), MSG_MORE);
    if (header_bytes_sent != (ssize_t)str.length()) {
        return Pistache::Async::Promise<ssize_t>::rejected(
            Pistache::Error("Unable to send HTTP response header"));
    }

    return Pistache::Async::Promise<ssize_t>::resolved(header_bytes_sent);
}

Pistache::Async::Promise<ssize_t>
send_pcap_response_header(Pistache::Http::ResponseWriter& response,
                          transfer_context& context)
{
    return send_pcap_response_header(
        response,
        dynamic_cast<pcap_transfer_context&>(context).get_total_length());
}

class pcap_thread_transfer_context : public pcap_transfer_context
{
public:
    pcap_thread_transfer_context(std::shared_ptr<Pistache::Tcp::Peer> peer,
                                 Pistache::Tcp::Transport* transport,
                                 bool chunked = false)
        : m_transport(transport)
        , m_peer(peer)
        , m_writer(std::make_unique<pcap_buffer_writer>())
        , m_buffer_sent(0)
        , m_total_bytes_sent(0)
        , m_chunked(chunked)
        , m_error(false)
    {}

    virtual ~pcap_thread_transfer_context()
    {
        if (m_thread) {
            // Make sure thread is dead
            m_thread->join();
        }
    }

    size_t get_total_length() const override
    {
        // Currently code will enable chunk encoding if returned length is 0
        if (m_chunked) return 0;
        return calc_pcap_file_length(*m_reader);
    }

    Pistache::Async::Promise<ssize_t> start() override
    {
        auto promise = Pistache::Async::Promise<ssize_t>(
            [&](Pistache::Async::Deferred<ssize_t> deferred) mutable {
                m_deferred = std::move(deferred);
            });

        m_total_bytes_sent = 0;
        m_buffer_sent = 0;
        m_error = false;

        m_reader->rewind();

        // Disable the peer so Pistache doesn't do anything with it while
        // it is being used in the transfer thread.
        transport_peer_disable(*m_transport, *m_peer);

        // Send the Capture data from a worker thread
        m_thread = std::make_unique<std::thread>([&]() {
            transfer();

            // Exec peer enable in Pistache transport reactor thread
            transport_exec(*m_transport, [&]() {
                transport_peer_enable(*m_transport, *m_peer);
            });
        });

        return promise;
    }

private:
    void transfer()
    {
        m_writer->write_file_header();

        std::array<capture_packet*, 16> packets;
        while (!m_reader->is_done() && !m_error) {
            auto n = m_reader->read_packets(packets.data(), packets.size());
            std::for_each(packets.data(),
                          packets.data() + n,
                          [&](auto& packet) { write_packet(*packet); });
        }

        if (!m_error) {
            if (!m_writer->write_file_trailer()) {
                flush();
                if (!m_writer->write_file_trailer()) {
                    OP_LOG(OP_LOG_ERROR, "Failed writing pcap trailer");
                }
            }
            flush();
            if (m_chunked) { write_chunk_end(); }
        }

        if (m_error) {
            // Error writing data
            m_deferred.reject(Pistache::Error("Failed sending pcap data"));
            m_done = true;
            return;
        }

        m_deferred.resolve((ssize_t)m_total_bytes_sent);
        m_done = true;
    }

    bool write_packet(const capture_packet& packet)
    {
        enhanced_packet_block block_hdr;

        block_hdr.block_type = block_type::ENHANCED_PACKET;
        block_hdr.interface_id = 0;
        block_hdr.timestamp_high = packet.hdr.timestamp >> 32;
        block_hdr.timestamp_low = packet.hdr.timestamp;
        block_hdr.captured_len = packet.hdr.captured_len;
        block_hdr.packet_len = packet.hdr.packet_len;
        block_hdr.block_total_length = pad_block_length(
            sizeof(block_hdr) + sizeof(uint32_t) + packet.hdr.captured_len);

        if (block_hdr.block_total_length > m_writer->get_available_length()) {
            if (!flush()) { return false; }
            if (block_hdr.block_total_length
                > m_writer->get_available_length()) {
                // Packet is too big for buffer so just send directly
                return send_header_and_data(
                    reinterpret_cast<uint8_t*>(&block_hdr),
                    sizeof(block_hdr),
                    packet.data,
                    packet.hdr.captured_len);
            }
        }
        return m_writer->write_packet_block(block_hdr, packet.data);
    }

    bool write_chunk_end()
    {
        auto last_chunk_str = get_last_chunk_str();
        auto n = send_to_peer_timeout(
            *m_peer, last_chunk_str.c_str(), last_chunk_str.length(), 0);
        if (n < 0) {
            m_error = true;
            return false;
        }
        return true;
    }

    bool flush()
    {
        auto length = m_writer->get_length();
        if (length == 0) return true;

        auto remain = length - m_buffer_sent;
        if (remain) {
            ssize_t nsent = 0;
            if (m_chunked) {
                auto chunk_header_str = get_chunk_header_str(remain);
                if (send_to_peer_timeout(*m_peer,
                                         chunk_header_str.c_str(),
                                         chunk_header_str.length(),
                                         MSG_MORE)
                    != (int)chunk_header_str.length()) {
                    m_error = true;
                    return false;
                }

                // If all data doesn't get sent then need to treat it as an
                // error because we already said how much we were going to write
                nsent = send_to_peer_timeout(
                    *m_peer, m_writer->get_data() + m_buffer_sent, remain, 0);
                if (nsent != (int)remain) {
                    m_error = true;
                    return false;
                }

                auto chunk_trailer_str = get_chunk_trailer_str();
                if (send_to_peer_timeout(*m_peer,
                                         chunk_trailer_str.c_str(),
                                         chunk_trailer_str.length(),
                                         0)
                    != (int)chunk_trailer_str.length()) {
                    m_error = true;
                    return false;
                }
            } else {
                // If all data doesn't get sent then not necessarily an error
                // because data will still be in the buffer.
                nsent = send_to_peer_timeout(
                    *m_peer, m_writer->get_data() + m_buffer_sent, remain, 0);
                if (nsent < 0) {
                    m_error = true;
                    return false;
                }
            }

            m_buffer_sent += nsent;
            m_total_bytes_sent += nsent;
        }
        if (m_buffer_sent == length) {
            m_writer->flush();
            m_buffer_sent = 0;
            return true;
        }
        return false;
    }

    bool send_header_and_data(const uint8_t* hdr,
                              size_t hdr_len,
                              const uint8_t* data,
                              size_t data_len)
    {
        uint32_t total_block_len = pad_block_length(hdr_len + data_len);
        auto pad_len = (total_block_len - hdr_len - data_len);

        if (m_chunked) {
            auto chunk_header_str = get_chunk_header_str(total_block_len);
            if (send_to_peer_timeout(*m_peer,
                                     chunk_header_str.c_str(),
                                     chunk_header_str.length(),
                                     MSG_MORE)
                != (int)chunk_header_str.length()) {
                m_error = true;
                return false;
            }
        }

        if (send_to_peer_timeout(*m_peer, (void*)hdr, hdr_len, MSG_MORE)
            != (ssize_t)hdr_len) {
            m_error = true;
            return false;
        }
        m_total_bytes_sent += hdr_len;
        if (send_to_peer_timeout(*m_peer, (void*)data, data_len, 0)
            != (ssize_t)data_len) {
            m_error = true;
            return false;
        }
        m_total_bytes_sent += data_len;
        if (pad_len) {
            uint64_t pad = 0;
            if (send_to_peer_timeout(*m_peer, (void*)&pad, pad_len, MSG_MORE)
                != (ssize_t)pad_len) {
                m_error = true;
                return false;
            }
            m_total_bytes_sent += pad_len;
        }
        if (send_to_peer_timeout(
                *m_peer, (void*)&total_block_len, sizeof(total_block_len), 0)
            != (ssize_t)sizeof(total_block_len)) {
            m_error = true;
            return false;
        }
        m_total_bytes_sent += sizeof(total_block_len);
        if (m_chunked) {
            auto chunk_trailer_str = get_chunk_trailer_str();
            if (send_to_peer_timeout(*m_peer,
                                     chunk_trailer_str.c_str(),
                                     chunk_trailer_str.length(),
                                     0)
                != (int)chunk_trailer_str.length()) {
                m_error = true;
                return false;
            }
        }
        return true;
    }

    std::unique_ptr<std::thread> m_thread;
    Pistache::Tcp::Transport* m_transport;
    std::shared_ptr<Pistache::Tcp::Peer> m_peer;
    std::unique_ptr<pcap_buffer_writer> m_writer;
    size_t m_buffer_sent;
    size_t m_total_bytes_sent;
    bool m_chunked;
    bool m_error;
};

class pcap_async_transfer_context : public pcap_transfer_context
{
public:
    pcap_async_transfer_context(std::shared_ptr<Pistache::Tcp::Peer> peer,
                                Pistache::Tcp::Transport* transport,
                                bool chunked = false)
        : m_transport(transport)
        , m_peer(peer)
        , m_writer(std::make_unique<pcap_buffer_writer>())
        , m_total_bytes_sent(0)
        , m_chunked(chunked)
        , m_error(false)
        , m_wrote_trailer(false)
    {}

    virtual ~pcap_async_transfer_context() {}

    size_t get_total_length() const override
    {
        // Currently code will enable chunk encoding if returned length is 0
        if (m_chunked) return 0;
        return calc_pcap_file_length(*m_reader);
    }

    Pistache::Async::Promise<ssize_t> start() override
    {
        auto promise = Pistache::Async::Promise<ssize_t>(
            [&](Pistache::Async::Deferred<ssize_t> deferred) mutable {
                m_deferred = std::move(deferred);
            });

        m_total_bytes_sent = 0;
        m_error = false;
        m_wrote_trailer = false;

        m_reader->rewind();
        m_burst = std::make_unique<burst_reader_type>(m_reader.get());

        m_writer->write_file_header();
        transfer();

        return promise;
    }

private:
    Pistache::Async::Promise<ssize_t> flush()
    {
        auto length = m_writer->get_length();

        if (length == 0) {
            return Pistache::Async::Promise<ssize_t>::resolved(0);
        }

        Pistache::RawBuffer buffer((char*)m_writer->get_data(), length);
        m_writer->flush();
        if (!m_chunked) {
            return m_transport->asyncWrite(m_peer->fd(), buffer)
                .then(
                    [&](ssize_t length) {
                        m_total_bytes_sent += length;
                        return transfer();
                    },
                    [&](std::exception_ptr& eptr) {
                        m_deferred.reject(
                            Pistache::Error("Failed sending pcap data"));
                        m_done = true;
                        m_error = true;
                        return Pistache::Async::Promise<ssize_t>::rejected(
                            eptr);
                    });
        } else {
            auto chunk_header_str = get_chunk_header_str(buffer.size());

            return m_transport
                ->asyncWrite(m_peer->fd(),
                             Pistache::RawBuffer(chunk_header_str,
                                                 chunk_header_str.length()),
                             MSG_MORE)
                .then(
                    [&](ssize_t) {
                        return m_transport->asyncWrite(m_peer->fd(), buffer);
                    },
                    [&](std::exception_ptr& eptr) {
                        m_deferred.reject(
                            Pistache::Error("Failed sending chunk header"));
                        m_done = true;
                        m_error = true;
                        return Pistache::Async::Promise<ssize_t>::rejected(
                            eptr);
                    })
                .then(
                    [&](ssize_t) {
                        auto chunk_trailer_str = get_chunk_trailer_str();
                        return m_transport->asyncWrite(
                            m_peer->fd(),
                            Pistache::RawBuffer(chunk_trailer_str,
                                                chunk_trailer_str.length()));
                    },
                    [&](std::exception_ptr& eptr) {
                        m_deferred.reject(
                            Pistache::Error("Failed sending pcap data"));
                        m_done = true;
                        m_error = true;
                        return Pistache::Async::Promise<ssize_t>::rejected(
                            eptr);
                    })
                .then([&](ssize_t) { return transfer(); },
                      [&](std::exception_ptr& eptr) {
                          m_deferred.reject(
                              Pistache::Error("Failed sending chunk trailer"));
                          m_done = true;
                          m_error = true;
                          return Pistache::Async::Promise<ssize_t>::rejected(
                              eptr);
                      });
        }
    }

    Pistache::Async::Promise<ssize_t> transfer()
    {
        bool is_done = m_burst->is_done();
        if (!is_done) {
            // Fill the buffer
            while (!is_done) {
                auto packet = *(m_burst->get());
                if (!m_writer->write_packet(packet)) break;
                is_done = m_burst->next();
            }

            if (m_reader->is_done()) {
                if (m_writer->write_file_trailer()) { m_wrote_trailer = true; }
            }

            // Send the buffer
            if (m_writer->get_length() != 0) { return flush(); }
        }

        if (!m_wrote_trailer) {
            if (m_writer->write_file_trailer()) { m_wrote_trailer = true; }

            // Send the buffer
            if (m_writer->get_length() != 0) { return flush(); }
        }

        if (m_chunked) {
            auto last_chunk_str = get_last_chunk_str();
            return m_transport->asyncWrite(
                m_peer->fd(),
                Pistache::RawBuffer(last_chunk_str, last_chunk_str.length()));
        }

        m_deferred.resolve(m_total_bytes_sent);
        auto result =
            Pistache::Async::Promise<ssize_t>::resolved(m_total_bytes_sent);

        m_done = true;
        return result;
    }

    using burst_reader_type =
        capture_burst_reader<capture_buffer_reader*, capture_packet*>;

    Pistache::Tcp::Transport* m_transport;
    std::shared_ptr<Pistache::Tcp::Peer> m_peer;
    std::unique_ptr<pcap_buffer_writer> m_writer;
    std::unique_ptr<burst_reader_type> m_burst;
    ssize_t m_total_bytes_sent;
    bool m_chunked;
    bool m_error;
    bool m_wrote_trailer;
};

std::unique_ptr<transfer_context>
create_pcap_transfer_context(Pistache::Http::ResponseWriter& response)
{
#if 1
    std::unique_ptr<transfer_context> context{new pcap_thread_transfer_context(
        response.peer(), get_transport(response))};
#else
    std::unique_ptr<transfer_context> context{new pcap_async_transfer_context(
        response.peer(), get_transport(response))};
#endif
    return context;
}

Pistache::Async::Promise<ssize_t> serve_capture_pcap(transfer_context& context)
{
    return dynamic_cast<pcap_transfer_context&>(context).start();
}

} // namespace openperf::packet::capture::pcap