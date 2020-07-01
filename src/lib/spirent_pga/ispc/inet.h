/*
 * XXX: If we ever support big-endian targets, these functions will
 * need to be updated.
 */
inline unsigned int32 ntohl(unsigned int32 x)
{
    return ((x & 0x000000ff) << 24 | ((x & 0x0000ff00) << 8)
            | ((x & 0x00ff0000) >> 8) | ((x & 0xff000000) >> 24));
}

inline uniform unsigned int32 ntohl(uniform unsigned int32 x)
{
    return ((x & 0x000000ff) << 24 | ((x & 0x0000ff00) << 8)
            | ((x & 0x00ff0000) >> 8) | ((x & 0xff000000) >> 24));
}

inline unsigned int32 htonl(unsigned int32 x) { return (ntohl(x)); }

inline uniform unsigned int32 htonl(uniform unsigned int32 x)
{
    return (ntohl(x));
}
