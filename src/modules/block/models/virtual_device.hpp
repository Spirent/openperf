#ifndef _OP_BLOCK_VIRTUAL_DEVICE_HPP_
#define _OP_BLOCK_VIRTUAL_DEVICE_HPP_

namespace openperf::block {

class virtual_device {
protected:
    int fd = -1;
public:
    virtual ~virtual_device() {}
    virtual int vopen() = 0;
    virtual void vclose() = 0;
    int get_fd() {
        return fd;
    }
};

}

#endif