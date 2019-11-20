#ifndef _OP_SOCKET_UNIX_SOCKET_H_
#define _OP_SOCKET_UNIX_SOCKET_H_

#include <string>

namespace openperf {
namespace socket {

class unix_socket
{
    std::string m_path;
    int m_fd;

public:
    unix_socket(const std::string_view path, int type);
    ~unix_socket();
    int get();
};

}
}

#endif /* _OP_SOCKET_UNIX_SOCKET_H_ */
