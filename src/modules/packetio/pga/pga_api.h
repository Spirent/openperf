#ifndef _ICP_PACKETIO_PGA_API_H_
#define _ICP_PACKETIO_PGA_API_H_

#include <string>
#include <optional>
#include <variant>

#include "packetio/pga/generic_sink.h"
#include "packetio/pga/generic_source.h"

namespace icp::packetio::pga::api {

extern std::string_view endpoint;

struct request_add_interface_sink {
    std::string id;
    generic_sink item;
};

struct request_del_interface_sink {
    std::string id;
    generic_sink item;
};

struct request_add_interface_source {
    std::string id;
    generic_source item;
};

struct request_del_interface_source {
    std::string id;
    generic_source item;
};

struct request_add_port_sink {
    std::string id;
    generic_sink item;
};

struct request_del_port_sink {
    std::string id;
    generic_sink item;
};

struct request_add_port_source {
    std::string id;
    generic_source item;
};

struct request_del_port_source {
    std::string id;
    generic_source item;
};

typedef std::variant<request_add_interface_sink,
                     request_del_interface_sink,
                     request_add_interface_source,
                     request_del_interface_source,
                     request_add_port_sink,
                     request_del_port_sink,
                     request_add_port_source,
                     request_del_port_source> request_msg;

struct reply_ok {};

struct reply_error {
    int code;
    std::string mesg;
};

typedef std::variant<reply_ok,
                     reply_error> reply_msg;


reply_msg submit_request(void* socket, const request_msg request);

std::optional<request_msg> recv_request(void* socket, int flags);
int send_reply(void* socket, const reply_msg reply);

}

#endif /* _ICP_PACKETIO_PGA_API_H_ */
