#ifndef _REGURGITATE_IMPL_LOAD_STORE_FP64_H_
#define _REGURGITATE_IMPL_LOAD_STORE_FP64_H_

#include "bitonic_centroid_sort_common.h"
#include "load_fp.h"

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
        vals[i] = doublebits(c[j].vals);
        j++;
    }
}

#endif /* _REGURGITATE_IMPL_LOAD_STORE_FP64_H_ */
