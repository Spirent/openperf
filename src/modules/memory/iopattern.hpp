#ifndef _OP_MEMORY_IOPATTERN_HPP_
#define _OP_MEMORY_IOPATTERN_HPP_

#include <string>

namespace openperf::memory {

class iopattern 
{
public:
    enum pattern_type {
        RANDOM,
        SEQUENTIAL,
        REVERSE
    };

private:
    pattern_type _value;

public:
    iopattern(std::string& str) {
        if (str == "random")
            _value = RANDOM;
        else if (str == "sequential")
            _value = SEQUENTIAL;
        else if (str == "reverse")
            _value = REVERSE;
    }
};

}

#endif // _OP_MEMORY_IOPATTERN_HPP_
