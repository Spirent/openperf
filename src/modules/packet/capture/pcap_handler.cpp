#include <zmq.h>
#include <sys/stat.h>
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

Pistache::Async::Promise<ssize_t>
send_pcap_response_header_async(Pistache::Http::ResponseWriter& response,
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

    return transport->asyncWrite(peer->fd(), buf->buffer(), MSG_MORE);
}

Pistache::Async::Promise<ssize_t>
send_pcap_response_header_async(Pistache::Http::ResponseWriter& response,
                                std::shared_ptr<transfer_context> context)
{
    return send_pcap_response_header_async(response,
                                           context->get_total_length());
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
    std::ostringstream os;

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
    // Since peer is not supposed to write to the connection this should only
    // be an issue if the connection is closed.

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
                          std::shared_ptr<transfer_context> context)
{
    return send_pcap_response_header(response, context->get_total_length());
}

class pcap_buffered_writer
{
public:
    pcap_buffered_writer() {}

    size_t calc_length(capture_buffer_reader& reader)
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
        reader.rewind();

        return length;
    }

    bool write_start()
    {
        uint8_t* ptr = m_buffer.data();
        m_buffer_length = 0;

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
        return true;
    }

    bool write_packet_block(pcapng::enhanced_packet_block& block_hdr,
                            uint8_t* block_data)
    {
        uint8_t* ptr = m_buffer.data();
        auto remain = m_buffer.size() - m_buffer_length;
        if (block_hdr.block_total_length > remain) { return false; }
        std::copy(reinterpret_cast<uint8_t*>(&block_hdr),
                  reinterpret_cast<uint8_t*>(&block_hdr) + sizeof(block_hdr),
                  ptr + m_buffer_length);
        std::copy(block_data,
                  block_data + block_hdr.captured_len,
                  ptr + m_buffer_length + sizeof(block_hdr));
        m_buffer_length += block_hdr.block_total_length;
        std::copy(reinterpret_cast<uint8_t*>(&block_hdr.block_total_length),
                  reinterpret_cast<uint8_t*>(&block_hdr.block_total_length)
                      + sizeof(block_hdr.block_total_length),
                  ptr + m_buffer_length - sizeof(block_hdr.block_total_length));
        return true;
    }

    bool write_packet(capture_packet& packet)
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
        return write_packet_block(block_hdr, packet.data);
    }

    bool write_end() { return true; }

    uint8_t* get_data() { return m_buffer.data(); };
    size_t get_length() const { return m_buffer_length; }
    size_t get_available_length() const
    {
        return m_buffer.size() - m_buffer_length;
    }
    void flush() { m_buffer_length = 0; }

protected:
    std::array<uint8_t, 16384> m_buffer;
    size_t m_buffer_length = 0;
};

class pcap_transfer_context : public transfer_context
{
public:
    virtual ~pcap_transfer_context() = default;

    void set_reader(std::unique_ptr<capture_buffer_reader>& reader) override
    {
        m_reader.reset(reader.release());
    }

    virtual Pistache::Async::Promise<ssize_t> start() = 0;

protected:
    std::unique_ptr<capture_buffer_reader> m_reader;
    Pistache::Async::Deferred<ssize_t> m_deferred;
};

class pcap_thread_transfer_context : public pcap_transfer_context
{
public:
    pcap_thread_transfer_context(std::shared_ptr<Pistache::Tcp::Peer> peer,
                                 bool chunked = false)
        : m_peer(peer)
        , m_writer(std::make_unique<pcap_buffered_writer>())
        , m_buffer_sent(0)
        , m_total_bytes_sent(0)
        , m_error(false)
        , m_chunked(chunked)
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
        return m_writer->calc_length(*m_reader);
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

        // Send the Capture data from a thread
        m_thread = std::make_unique<std::thread>([&]() { transfer(); });
        return promise;
    }

private:
    void transfer()
    {
        m_writer->write_start();

        while (!m_reader->is_done() && !m_error) {
            m_reader->read([&](auto& packet) {
                if (!write_packet(packet)) return false;
                return true;
            });
        }

        if (!m_error) {
            m_writer->write_end();
            flush();
            if (m_chunked) { write_chunk_end(); }
        }

        if (m_error) {
            // Error writing data
            m_deferred.reject(Pistache::Error("Failed sending pcap data"));
            return;
        }

        // FIXME:
        // Need some way to enable the peer in pistache thread again.
        // This should be done through a message for thread safety.
        m_deferred.resolve((ssize_t)m_total_bytes_sent);
    }

    bool write_packet(capture_packet& packet)
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

        if (block_hdr.block_total_length > m_writer->get_available_length()) {
            if (!flush()) { return false; }
            if (block_hdr.block_total_length
                > m_writer->get_available_length()) {
                // Packet is too big for buffer so just send directly
                return send_header_and_data(
                    reinterpret_cast<uint8_t*>(&block_hdr),
                    sizeof(block_hdr),
                    packet.data,
                    packet.captured_len);
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
        uint32_t total_block_len = pcapng::pad_block_length(hdr_len + data_len);
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
    std::shared_ptr<Pistache::Tcp::Peer> m_peer;
    std::unique_ptr<pcap_buffered_writer> m_writer;
    size_t m_buffer_sent;
    size_t m_total_bytes_sent;
    bool m_error;
    bool m_chunked;
};

class pcap_async_transfer_context : public pcap_transfer_context
{
public:
    pcap_async_transfer_context(std::shared_ptr<Pistache::Tcp::Peer> peer,
                                Pistache::Tcp::Transport* transport,
                                bool chunked = false)
        : m_transport(transport)
        , m_peer(peer)
        , m_writer(std::make_unique<pcap_buffered_writer>())
        , m_total_bytes_sent(0)
        , m_chunked(chunked)
    {}

    virtual ~pcap_async_transfer_context() {}

    size_t get_total_length() const override
    {
        // Currently code will enable chunk encoding if returned length is 0
        if (m_chunked) return 0;
        return m_writer->calc_length(*m_reader);
    }

    Pistache::Async::Promise<ssize_t> start() override
    {
        auto promise = Pistache::Async::Promise<ssize_t>(
            [&](Pistache::Async::Deferred<ssize_t> deferred) mutable {
                m_deferred = std::move(deferred);
            });

        m_reader->rewind();

        m_writer->write_start();
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
                    Pistache::Async::Throw)
                .then(
                    [&](ssize_t) {
                        auto chunk_trailer_str = get_chunk_trailer_str();
                        return m_transport->asyncWrite(
                            m_peer->fd(),
                            Pistache::RawBuffer(chunk_trailer_str,
                                                chunk_trailer_str.length()));
                    },
                    Pistache::Async::Throw)
                .then([&](ssize_t) { return transfer(); },
                      Pistache::Async::Throw);
        }
    }

    Pistache::Async::Promise<ssize_t> transfer()
    {
        // Fill the buffer
        while (!m_reader->is_done()) {
            m_reader->read([&](auto& packet) {
                if (!m_writer->write_packet(packet)) return false;
                return true;
            });
        }

        if (m_writer->get_length() != 0) {
            if (m_reader->is_done()) { m_writer->write_end(); }
            return flush();
        }

        if (m_chunked) {
            auto last_chunk_str = get_last_chunk_str();
            return m_transport->asyncWrite(
                m_peer->fd(),
                Pistache::RawBuffer(last_chunk_str, last_chunk_str.length()));
        }

        m_deferred.resolve(m_total_bytes_sent);
        return Pistache::Async::Promise<ssize_t>::resolved(m_total_bytes_sent);
    }

    Pistache::Tcp::Transport* m_transport;
    std::shared_ptr<Pistache::Tcp::Peer> m_peer;
    std::unique_ptr<pcap_buffered_writer> m_writer;
    ssize_t m_total_bytes_sent;
    bool m_chunked;
};

std::shared_ptr<transfer_context>
create_pcap_transfer_context(Pistache::Http::ResponseWriter& response)
{
#if 1
    std::shared_ptr<transfer_context> context{
        new pcap_thread_transfer_context(response.peer())};
#else
    std::shared_ptr<transfer_context> context{new pcap_async_transfer_context(
        response.peer(), get_tcp_transport(response))};
#endif
    return context;
}

Pistache::Async::Promise<ssize_t>
serve_capture_pcap(std::shared_ptr<transfer_context> context)
{
    auto pcontext = dynamic_cast<pcap_transfer_context*>(context.get());
    assert(pcontext);
    return pcontext->start();
}

} // namespace openperf::packet::capture::api