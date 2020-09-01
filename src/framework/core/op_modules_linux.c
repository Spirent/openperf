#include <unistd.h>
#include <limits.h>

int op_find_executable_path(char* path)
{
    int rv;
    if ((rv = readlink("/proc/self/exe", path, PATH_MAX - 1)) == -1) return 1;
    path[rv] = 0;

    return 0;
}
