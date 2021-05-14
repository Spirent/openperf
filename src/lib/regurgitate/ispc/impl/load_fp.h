#ifndef _REGURGITATE_IMPL_LOAD_FP_H_
#define _REGURGITATE_IMPL_LOAD_FP_H_

#include "bitonic_centroid_sort_common.h"

static inline void load_chunk(centroid c[programCount],
                              uniform key_type keys[],
                              uniform val_type vals[],
                              uniform uint32 length)
{
    uniform int j = 0;
    foreach(i = 0 ... programCount * programCount)
    {
        c[j].keys = select(i < length, keys[i], (key_type)KEY_MAX);
        c[j].vals = select(i < length, intbits(vals[i]), (int_val_type)0);
        j++;
    }
}

#endif /* _REGURGITATE_IMPL_LOAD_FP_H_ */
