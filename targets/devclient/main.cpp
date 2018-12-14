#include <sys/eventfd.h>
#include <unistd.h>
#include "socket/client/api_client.h"

int main()
{
    auto& client = icp::socket::api::client::instance();
    client.init();

    auto fd = client.socket(AF_INET, SOCK_DGRAM, 0);
    printf("Opened fd = %d\n", fd);
    client.close(fd);

    return (EXIT_SUCCESS);
}
