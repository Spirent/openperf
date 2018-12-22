#include <arpa/inet.h>
#include <unistd.h>
#include <vector>

#include "catch.hpp"

#include "socket/client/api_client.h"

TEST_CASE("UDP socket API checks", "[udp socket]")
{
    auto& client = icp::socket::api::client::instance();

    SECTION("can create IPv4 UDP socket, ") {
        auto fd = client.socket(AF_INET, SOCK_DGRAM, 0);
        REQUIRE(fd > 0);
        client.close(fd);
    }

    SECTION("can create IPv6 UDP socket, ") {
        auto fd = client.socket(AF_INET6, SOCK_DGRAM, 0);
        REQUIRE(fd > 0);
        client.close(fd);
    }

    SECTION("can use {get,set}sockopt, ") {
        auto fd = client.socket(AF_INET, SOCK_DGRAM, 0);
        REQUIRE(fd > 0);

        std::vector<int> options = {SO_BROADCAST, SO_REUSEADDR};
        for (auto opt : options) {
            DYNAMIC_SECTION("verifying option = " << opt) {
                /* default value should be false */
                int flag = -1;
                socklen_t size = sizeof(flag);
                auto error = client.getsockopt(fd, SOL_SOCKET, opt, &flag, &size);
                REQUIRE(!error);
                REQUIRE(flag == 0);
                REQUIRE(size == sizeof(flag));

                /* now toggle the value */
                flag = 1;
                error = client.setsockopt(fd, SOL_SOCKET, opt, &flag, sizeof(flag));
                REQUIRE(!error);

                /* now check the new value */
                flag = -1;
                size = sizeof(flag);
                error = client.getsockopt(fd, SOL_SOCKET, opt, &flag, &size);
                REQUIRE(!error);
                REQUIRE(flag == 1);
            }
        }

        SECTION("check that unsupported option returns ENOPROTOOPT, ") {
            int flag = -1;
            socklen_t size = sizeof(flag);
            /* can't accept on UDP socket */
            auto error = client.getsockopt(fd, SOL_SOCKET, SO_ACCEPTCONN, &flag, &size);
            REQUIRE(error == -1);
            REQUIRE(errno == ENOPROTOOPT);
        }
        client.close(fd);
    }

    SECTION("can bind IPv4 address, ") {
        auto fd = client.socket(AF_INET, SOCK_DGRAM, 0);
        REQUIRE(fd > 0);

        struct sockaddr_storage sstorage;
        struct sockaddr_in *sa = reinterpret_cast<sockaddr_in*>(&sstorage);
        sa->sin_family = AF_INET;
        sa->sin_port = htons(35377);
        sa->sin_addr.s_addr = htonl(INADDR_ANY);

        auto error = client.bind(fd, reinterpret_cast<sockaddr*>(sa), sizeof(*sa));
        REQUIRE(!error);

        SECTION("can get bind address via getsockname, ") {
            struct sockaddr_storage sock_storage;
            socklen_t sock_length = sizeof(sock_storage);
            error = client.getsockname(fd, reinterpret_cast<sockaddr*>(&sock_storage),
                                       &sock_length);
            REQUIRE(!error);
            auto sock = reinterpret_cast<sockaddr_in*>(&sock_storage);
            REQUIRE(sock->sin_family == sa->sin_family);
            REQUIRE(sock->sin_port == sa->sin_port);
            REQUIRE(sock->sin_addr.s_addr == sa->sin_addr.s_addr);
            REQUIRE(sock_length == sizeof(sockaddr_in));
        }

        SECTION("cannot bind twice, ") {
            error = client.bind(fd, reinterpret_cast<sockaddr*>(sa), sizeof(*sa));
            REQUIRE(error == -1);
        }

        client.close(fd);
    }

    SECTION("can connect IPv4 address, ") {
        auto fd = client.socket(AF_INET, SOCK_DGRAM, 0);
        REQUIRE(fd > 0);

        struct sockaddr_storage sstorage;
        struct sockaddr_in *sa = reinterpret_cast<sockaddr_in*>(&sstorage);
        sa->sin_family = AF_INET;
        sa->sin_port = htons(12345);
        sa->sin_addr.s_addr = htonl(0x0A010101);

        auto error = client.connect(fd, reinterpret_cast<sockaddr*>(sa), sizeof(*sa));
        REQUIRE(!error);

        SECTION("can get client via getpeername, ") {
            struct sockaddr_storage peer_storage;
            socklen_t peer_length = sizeof(peer_storage);
            error = client.getpeername(fd, reinterpret_cast<sockaddr*>(&peer_storage),
                                       &peer_length);
            REQUIRE(!error);
            auto peer = reinterpret_cast<sockaddr_in*>(&peer_storage);
            REQUIRE(peer->sin_family == sa->sin_family);
            REQUIRE(peer->sin_port == sa->sin_port);
            REQUIRE(peer->sin_addr.s_addr == sa->sin_addr.s_addr);
            REQUIRE(peer_length == sizeof(sockaddr_in));
        }

        SECTION("can connect to IPv4 address again, ") {
            sa->sin_family = AF_INET;
            sa->sin_port = htons(54321);
            sa->sin_addr.s_addr = htonl(0x0A010102);

            error = client.connect(fd, reinterpret_cast<sockaddr*>(sa), sizeof(*sa));
            REQUIRE(!error);
        }

        SECTION("can disconnect, ") {
            sa->sin_family = AF_UNSPEC;
            error = client.connect(fd, reinterpret_cast<sockaddr*>(sa), sizeof(*sa));
            REQUIRE(!error);
        }

        client.close(fd);
    }

#if 0
    SECTION("can receive from bound socket, ") {
        auto fd = client.socket(AF_INET, SOCK_DGRAM, 0);
        REQUIRE(fd > 0);

        sockaddr_in sa;
        sa.sin_family = AF_INET;
        sa.sin_port = htons(3357);
        sa.sin_addr.s_addr = htons(INADDR_ANY);

        auto error = client.bind(fd, reinterpret_cast<sockaddr*>(&sa), sizeof(sa));
        REQUIRE(!error);

        sockaddr_in from;
        socklen_t fromlen = sizeof(from);
        uint8_t buffer[1024];
        auto result = client.recvfrom(fd, buffer, 1024, 0,
                                      reinterpret_cast<sockaddr*>(&from), &fromlen);
        REQUIRE(result > 0);

        buffer[result] = '\0';
        printf("Received %s bytes from %x:%d\n",
               buffer, htonl(from.sin_addr.s_addr), ntohs(from.sin_port));


        client.close(fd);
    }
#endif

    SECTION("can transmit on unconnected interface, ") {
        auto fd = client.socket(AF_INET, SOCK_DGRAM, 0);
        REQUIRE(fd > 0);

        sockaddr_in sa;
        sa.sin_family = AF_INET;
        sa.sin_port = htons(3357);
        sa.sin_addr.s_addr = htonl(0xc0a8fb57);

        const char *buffer = "Goodnight, moon!";
        auto result = client.sendto(fd, buffer, strlen(buffer), 0,
                                    reinterpret_cast<sockaddr*>(&sa), sizeof(sa));
        REQUIRE(result > 0);
        //printf("Sent buffer 1\n");
        const char *buffer2 = "Bye bye, birdie\n";
        result = client.sendto(fd, buffer2, strlen(buffer2), 0,
                               reinterpret_cast<sockaddr*>(&sa), sizeof(sa));
        REQUIRE(result > 0);
        //printf("Sent buffer 2\n");

        client.close(fd);
    }
}
