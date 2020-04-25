#ifndef _OP_PISTACHE_UTILS_HPP_
#define _OP_PISTACHE_UTILS_HPP_

#include <pistache/http.h>

namespace openperf::pistache_utils {

bool write_status_line(std::ostream& os,
                       Pistache::Http::Version version,
                       Pistache::Http::Code code);

bool write_headers(std::ostream& os,
                   const Pistache::Http::Header::Collection& headers);

bool write_cookies(std::ostream& os, const Pistache::Http::CookieJar& cookies);

std::string get_chunk_header_str(size_t chunk_size);

std::string get_chunk_trailer_str();

std::string get_last_chunk_str();

ssize_t send_to_peer(Pistache::Tcp::Peer& peer,
                     const void* data,
                     size_t len,
                     int flags);

ssize_t
send_to_peer_timeout(Pistache::Tcp::Peer& peer,
                     const void* data,
                     size_t len,
                     int flags,
                     const std::chrono::duration<int64_t, std::milli>& timeout =
                         std::chrono::duration<int64_t, std::milli>{5000});

Pistache::Tcp::Transport* get_transport(Pistache::Http::ResponseWriter& writer);

void transport_peer_disable(Pistache::Tcp::Transport& transport,
                            Pistache::Tcp::Peer& peer);

void transport_peer_enable(Pistache::Tcp::Transport& transport,
                           Pistache::Tcp::Peer& peer);

void transport_exec(Pistache::Tcp::Transport& transport,
                    std::function<void()>&& func);

} // namespace openperf::pistache_utils

#endif // _OP_PISTACHE_UTILS_HPP_