export void fill_constant_aligned(uniform unsigned int32 payload[],
                                  uniform unsigned int16 length,
                                  uniform unsigned int32 value)
{
    uniform int base = 0;
    foreach (i = 0 ... length) {
        base += packed_store_active(&payload[base], value);
    }
}
