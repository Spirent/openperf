export void
unpack_and_sum_indexicals(uniform unsigned int32 const fields[],
                          uniform unsigned int16 nb_fields,
                          uniform unsigned int32 const masks[],
                          uniform unsigned int16 nb_masks,
                          uniform unsigned int64* uniform counters[])
{
    foreach (i = 0 ... nb_fields)
    {
        for (uniform int j = 0; j < nb_masks; j++) {
            uniform int shift = count_trailing_zeros(masks[j]);
            int indexes = (fields[i] & masks[j]) >> shift;

            foreach_unique(idx in indexes) {
                counters[j][idx] += reduce_add(1);
            }
        }
    }
}
