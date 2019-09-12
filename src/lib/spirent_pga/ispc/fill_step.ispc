export uniform unsigned int8 fill_step_aligned(uniform unsigned int32 payload[],
                                               uniform unsigned int16 length,
                                               uniform unsigned int8 value,
                                               uniform int8 step)
{
    /* Initial fill value */
    unsigned int8<4> fill = { value + step * 4 * programIndex,
                              value + step * (4 * programIndex + 1),
                              value + step * (4 * programIndex + 2),
                              value + step * (4 * programIndex + 3) };

    /* Fill 'step' for each iteration of the loop */
    int8<4> fill_step = { step * 4 * programCount,
                          step * 4 * programCount,
                          step * 4 * programCount,
                          step * 4 * programCount };

    uniform int base = 0;
    foreach (i = 0 ... length) {
        base += packed_store_active(&payload[base], ((fill.x & 0xff)
                                                     | (fill.y & 0xff) << 8
                                                     | (fill.z & 0xff) << 16
                                                     | (fill.w & 0xff) << 24));
        fill += fill_step;
    }

    return (value + (step * length * 4));
}
