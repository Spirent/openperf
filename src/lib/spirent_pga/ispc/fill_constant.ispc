export void fill_constant_aligned(uniform unsigned int32 payload[],
                                  uniform unsigned int16 length,
                                  uniform unsigned int32 value)
{
    foreach (i = 0 ... length) {
        payload[i] = value;
    }
}
