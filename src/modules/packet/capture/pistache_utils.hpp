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

Pistache::Tcp::Transport*
get_tcp_transport(Pistache::Http::ResponseWriter& writer);

} // namespace openperf::pistache_utils

#endif // _OP_PISTACHE_UTILS_HPP_