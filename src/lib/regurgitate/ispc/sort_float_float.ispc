typedef float key_type;
typedef float val_type;
typedef int32 int_val_type;

#define KEY_MAX 340282346638528859811704183484516925440.000000

#include "impl/load_store_fp32.h"
#include "impl/bitonic_centroid_sort.h"

export void sort_float_float(uniform key_type means[],
                             uniform val_type weights[],
                             uniform uint32 length,
                             uniform key_type scratch_m[],
                             uniform val_type scratch_w[])
{
    bitonic_centroid_sort(means, weights, length, scratch_m, scratch_w);
}
