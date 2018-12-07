#include <sys/eventfd.h>
#include <unistd.h>
#include "socket/socket_api.h"
#include "socket/client.h"

int main()
{
    auto& client = icp::sock::client::instance();
    client.init();

    auto fd1 = client.socket(1, 1, 1);
    auto fd2 = client.socket(1, 1, 1);

    client.io_ping(fd1);
    client.io_ping(fd2);

    client.close(fd1);
    client.close(fd2);

    return (EXIT_SUCCESS);
}
