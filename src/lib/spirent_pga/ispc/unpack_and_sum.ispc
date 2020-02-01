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

            int idx = (fields[i] & masks[j]) >> shift;
            switch (idx) {
            case 0:
                counters[j][0] += reduce_add(1);
                break;
            case 1:
                counters[j][1] += reduce_add(1);
                break;
            case 2:
                counters[j][2] += reduce_add(1);
                break;
            case 3:
                counters[j][3] += reduce_add(1);
                break;
            case 4:
                counters[j][4] += reduce_add(1);
                break;
            case 5:
                counters[j][5] += reduce_add(1);
                break;
            case 6:
                counters[j][6] += reduce_add(1);
                break;
            case 7:
                counters[j][7] += reduce_add(1);
                break;
            case 8:
                counters[j][8] += reduce_add(1);
                break;
            case 9:
                counters[j][9] += reduce_add(1);
                break;
            case 10:
                counters[j][10] += reduce_add(1);
                break;
            case 11:
                counters[j][11] += reduce_add(1);
                break;
            case 12:
                counters[j][12] += reduce_add(1);
                break;
            case 13:
                counters[j][13] += reduce_add(1);
                break;
            case 14:
                counters[j][14] += reduce_add(1);
                break;
            case 15:
                counters[j][15] += reduce_add(1);
                break;
            }
        }
    }
}
