#include "socket/client/api_client.h"

#define CATCH_CONFIG_RUNNER
#include "catch.hpp"

int main(int argc, char* argv[])
{
    auto& client = icp::socket::api::client::instance();
    client.init();

    return (Catch::Session().run(argc, argv));
}
