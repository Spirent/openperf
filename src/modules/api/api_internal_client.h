#ifndef _ICP_API_INTERNAL_CLIENT_H_
#define _ICP_API_INTERNAL_CLIENT_H_

namespace icp::api::client {

std::pair<Pistache::Http::Code, std::string> internal_api_get(std::string_view resource);
std::pair<Pistache::Http::Code, std::string> internal_api_post(std::string_view resource,
                                                               const std::string &body);

}  // namespace icp::api::client
#endif
