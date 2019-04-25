#include "catch.hpp"
#include <pistache/http.h>
#include <pistache/client.h>
#include <pistache/endpoint.h>

#include "api/api_internal_client.h"
#include "api/api_service.h"

using namespace icp::api::client;

struct handler : public Pistache::Http::Handler
{
    HTTP_PROTOTYPE(handler)

    void onRequest(const Pistache::Http::Request& request,
                   Pistache::Http::ResponseWriter writer) override
    {
        // Client functions sending the correct HTTP method is implicitly tested here.
        // Behavior is different depending on which method the server side sees.
        if (request.method() == Pistache::Http::Method::Get) {
            if (request.resource() == "/Ok")
                writer.send(Pistache::Http::Code::Ok, request.resource());
            else if (request.resource() == "/Not_Found")
                writer.send(Pistache::Http::Code::Not_Found, request.resource());

        } else if (request.method() == Pistache::Http::Method::Post) {
            if (request.resource() == "/Ok")
                writer.send(Pistache::Http::Code::Ok, request.body());
            else if (request.resource() == "/Not_Found")
                writer.send(Pistache::Http::Code::Not_Found, request.body());
        }
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
    const Pistache::Address address("localhost", Pistache::Port(icp::api::api_get_service_port()));

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
