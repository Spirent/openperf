#include "catch.hpp"
#include <pistache/http.h>
#include <pistache/client.h>
#include <pistache/endpoint.h>

#include "api/api_internal_client.h"
#include "api/api_service.h"

using namespace openperf::api::client;

struct handler : public Pistache::Http::Handler
{
    HTTP_PROTOTYPE(handler)

    // Map resources to the expected return code.
    const std::map<std::string, Pistache::Http::Code> resource_code_map {
      {"/Ok", Pistache::Http::Code::Ok},
      {"/Not_Found", Pistache::Http::Code::Not_Found}};

    void onRequest(const Pistache::Http::Request& request,
                   Pistache::Http::ResponseWriter writer) override
    {
        // Client functions sending the correct HTTP method is implicitly tested here.
        // Behavior is different depending on which method the server side sees.
        const std::map<Pistache::Http::Method, std::string> method_body_map {
          {Pistache::Http::Method::Get, request.resource()},
          {Pistache::Http::Method::Post, request.body()}};

        writer.send(resource_code_map.at(request.resource()), method_body_map.at(request.method()));
    }
};

TEST_CASE("check internal API client support", "[api client]")
{
    // Creating a single huge test case because we're opening an actual TCP port and opening and
    // closing it multiple times requires timeouts that will slow down the unit tests. Besides,
    // there's no clear need to start and stop a server that has no state and merely reflects
    // requests back to the client.

    // Mock up the server side of things.
    // Taken from the Pistache unit tests.
    const Pistache::Address address("localhost", Pistache::Port(openperf::api::api_get_service_port()));

    Pistache::Http::Endpoint server(address);
    auto flags       = Pistache::Tcp::Options::ReuseAddr;
    auto server_opts = Pistache::Http::Endpoint::options().flags(flags);
    server.init(server_opts);
    server.setHandler(Pistache::Http::make_handler<handler>());
    server.serveThreaded();

    // Test GET method.
    auto result = internal_api_get("/Ok");
    REQUIRE(result.first == Pistache::Http::Code::Ok);
    REQUIRE(result.second == "/Ok");

    result = internal_api_get("/Not_Found");
    REQUIRE(result.first == Pistache::Http::Code::Not_Found);
    REQUIRE(result.second == "/Not_Found");

    // Test POST method.
    result = internal_api_post("/Ok", "This is a good body.");
    REQUIRE(result.first == Pistache::Http::Code::Ok);
    REQUIRE(result.second == "This is a good body.");

    result = internal_api_post("/Not_Found", "This is a missing body.");
    REQUIRE(result.first == Pistache::Http::Code::Not_Found);
    REQUIRE(result.second == "This is a missing body.");

    // This must be called after all operations are complete.
    server.shutdown();
}
