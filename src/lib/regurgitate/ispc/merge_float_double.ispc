typedef float key_type;
typedef double val_type;
typedef float scale_type;
typedef double sum_type;
typedef double weight_type;

#include "impl/digest_merge.h"

export uniform uint32 merge_float_double(uniform key_type in_means[],
                                         uniform val_type in_weights[],
                                         uniform uint32 in_length,
                                         uniform key_type out_means[],
                                         uniform val_type out_weights[],
                                         uniform uint32 out_length)
{
    return (merge(
        in_means, in_weights, in_length, out_means, out_weights, out_length));
}
