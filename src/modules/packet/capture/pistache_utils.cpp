#include "packet/capture/pistache_utils.hpp"

namespace openperf::pistache_utils {

bool write_status_line(std::ostream& os,
                       Pistache::Http::Version version,
                       Pistache::Http::Code code)
{
    os << version << ' ' << static_cast<int>(code) << ' ' << code
       << Pistache::Http::crlf;

    return !os.bad();
}

bool write_headers(std::ostream& os,
                   const Pistache::Http::Header::Collection& headers)
{
    for (const auto& header : headers.list()) {
        os << header->name() << ": ";
        header->write(os);
        os << Pistache::Http::crlf;
        if (!os) return false;
    }

    return true;
}

bool write_cookies(std::ostream& os, const Pistache::Http::CookieJar& cookies)
{
    for (const auto& cookie : cookies) {
        os << "Set-Cookie: " << cookie << Pistache::Http::crlf;
        if (!os) return false;
    }

    return true;
}

Pistache::Tcp::Transport*
get_tcp_transport(Pistache::Http::ResponseWriter& writer)
{
    // transport_ is private and there is no getter function
    // so get it using an identical class with public members
    class ResponseWriterDoppleganger : public Pistache::Http::Response
    {
    public:
        std::weak_ptr<Pistache::Tcp::Peer> peer_;
        Pistache::DynamicStreamBuf buf_;
        Pistache::Tcp::Transport* transport_;
        Pistache::Http::Timeout timeout_;
    };
    static_assert(sizeof(ResponseWriterDoppleganger)
                  == sizeof(Pistache::Http::ResponseWriter));

    return reinterpret_cast<ResponseWriterDoppleganger*>(&writer)->transport_;
}

} // namespace openperf::pistache_utils
