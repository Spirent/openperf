#ifndef _OP_API_SERVICE_HPP_
#define _OP_API_SERVICE_HPP_

namespace openperf::api {

/**
 * Get API service TCP port number
 */
in_port_t api_get_service_port(void);

/**
 * Get API service IP address string using for client connections
 * @return IPv4 address, IPv6 address or "localhost".
 */
std::string api_get_service_transport_address(void);

} // namespace openperf::api

#endif
