#ifndef _REGURGITATE_IMPL_LOAD_STORE_INT_H_
#define _REGURGITATE_IMPL_LOAD_STORE_INT_H_

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
        c[j].vals = select(i < length, vals[i], (val_type)0);
        j++;
    }
}

static inline void store_chunk(centroid c[programCount],
                               uniform key_type keys[],
                               uniform val_type vals[],
                               uniform uint32 length)
{
    assert(length <= programCount * programCount);

    uniform int j = 0;
    foreach(i = 0 ... length)
    {
        keys[i] = c[j].keys;
        vals[i] = c[j].vals;
        j++;
    }
}

#endif /* _REGURGITATE_IMPL_LOAD_STORE_INT_H_ */
