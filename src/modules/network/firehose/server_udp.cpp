

int udp_read_callback(int fd)
{
    ssize_t recv_or_err = 0;
    char recv_buffer[icp_firehose_request_size];
    size_t recv_length = sizeof(recv_buffer);
    char* recv_cursor = &recv_buffer[0];
    struct sockaddr_storage client_storage;
    struct sockaddr* client = (struct sockaddr*)&client_storage;
    socklen_t client_length = sizeof(struct sockaddr_in6);

    OP_LOG(OP_LOG_INFO, "Listening UDP\n");
    while ((recv_or_err = recvfrom(
                fd, recv_buffer, recv_length, 0, client, &client_length))
           != -1) {

        recv_cursor = &recv_buffer[0];
        OP_LOG(OP_LOG_INFO, "Received %zd UDP bytes\n", recv_or_err);
    }
    OP_LOG(OP_LOG_INFO, "Done listening UDP\n");
    return (0);
}

static int _server(int domain, protocol_t protocol, in_port_t port)
{
    struct sockaddr_storage client_storage;
    struct sockaddr* server_ptr = (struct sockaddr*)&client_storage;
    std::string domain_str;

    switch (domain) {
    case AF_INET: {
        struct sockaddr_in* server4 = (struct sockaddr_in*)server_ptr;
        server4->sin_family = AF_INET;
        server4->sin_port = htons(port);
        server4->sin_addr.s_addr = htonl(INADDR_ANY);
        domain_str = "IPv4";
        break;
    }
    case AF_INET6: {
        struct sockaddr_in6* server6 = (struct sockaddr_in6*)server_ptr;
        server6->sin6_family = AF_INET6;
        server6->sin6_port = htons(port);
        server6->sin6_addr = in6addr_any;
        domain_str = "IPv6";
        break;
    }
    default:
        return (-EINVAL);
    }

    int sock = 0, enable = true;
    switch (protocol) {
    case protocol_t::TCP:
        if ((sock = socket(domain, SOCK_STREAM, 0)) == -1) {
            OP_LOG(OP_LOG_WARNING,
                   "Unable to open %s TCP server socket: %s\n",
                   domain_str.c_str(),
                   strerror(errno));
            return -1;
        }
        break;
    case protocol_t::UDP:
        if ((sock = socket(domain, SOCK_DGRAM, 0)) == -1) {
            OP_LOG(OP_LOG_WARNING,
                   "Unable to open %s UDP server socket: %s\n",
                   domain_str.c_str(),
                   strerror(errno));
            return -1;
        }
        break;
    default:
        return (-EINVAL);
    }

    if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(enable))
        != 0) {
        ::close(sock);
        return (-1);
    }

    if (bind(sock, server_ptr, _get_sa_len(server_ptr)) == -1) {
        OP_LOG(OP_LOG_ERROR,
               "Unable to bind to socket (domain = %d, protocol = %d): %s\n",
               domain,
               protocol,
               strerror(errno));
    }

    if (protocol == protocol_t::TCP) {
        if (listen(sock, tcp_backlog) == -1) {
            OP_LOG(OP_LOG_ERROR,
                   "Unable to listen on socket %d: %s\n",
                   sock,
                   strerror(errno));
        }
    }

    return (sock);
}

void firehose_server_tcp::run_worker_thread()
{
    m_worker_thread = std::thread([&] {
        // Set the thread name
        op_thread_setname("op_net_srv_w");

        char recv_buffer[firehose::net_request_size];
        char* recv_cursor = &recv_buffer[0];
        size_t recv_length = sizeof(recv_buffer);
        socklen_t client_length = sizeof(sockaddr_in6);

        while (m_stopped.load(std::memory_order_relaxed)) {
            for (auto conn : m_connections) {
                ssize_t recv_or_err = 0;
                while ((recv_or_err = recvfrom(conn.fd,
                                               recv_buffer,
                                               recv_length,
                                               0,
                                               &conn.client,
                                               &client_length))
                       != -1) {
                    size_t bytes_left = recv_or_err;
                    auto req = firehose::parse_request(recv_cursor, bytes_left);
                    if (!req) {
                        char ntopbuf[INET6_ADDRSTRLEN];
                        const char* addr = inet_ntop(conn.client.sa_family,
                                                     get_sa_addr(&conn.client),
                                                     ntopbuf,
                                                     INET6_ADDRSTRLEN);
                        OP_LOG(OP_LOG_ERROR,
                               "Invalid firehose request received from %s:%d\n",
                               addr ? addr : "unknown",
                               ntohs(get_sa_port(&conn.client)));
                        OP_LOG(OP_LOG_INFO,
                               "recv = %zu, bytes_left = %zu\n",
                               recv_or_err,
                               bytes_left);
                        return (0);
                    }
                }
            }
        }
    });
}