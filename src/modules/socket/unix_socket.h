#ifndef _ICP_SOCKET_UNIX_SOCKET_H_
#define _ICP_SOCKET_UNIX_SOCKET_H_

#include <string>

namespace icp {
namespace socket {

class unix_socket
{
    std::string m_path;
    int m_fd;

public:
    unix_socket(const std::string_view path, int type);
    ~unix_socket();
    int get();

    static bool exists(const std::string_view path);
    static void remove(const std::string_view path);
};

}
}

#endif /* _ICP_SOCKET_UNIX_SOCKET_H_ */
