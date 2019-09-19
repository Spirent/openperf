#include "aos_soa_5.h"

inline static unsigned int32 fold(unsigned int32 x)
{
    return ((x >> 16) + (x & 0xffff));
}

export void checksum_ipv4_headers(const unsigned int8* const uniform ipv4_header_ptrs[],
                                  uniform unsigned int16 count,
                                  unsigned int32 uniform checksums[])
{
    uniform int32 scratch[5 * programCount];
    uniform int base = 0;

    foreach (i = 0 ... count) {
        /* Copy IPv4 header into scratch area */
        memcpy(&scratch[5 * programIndex], (unsigned int8* const)ipv4_header_ptrs[i], 20);

        /* Transpose it into our lanes */
        unsigned int32<5> headers;
        aos_to_soa5(&scratch[0],
                    (varying int32*)&headers[0],
                    (varying int32*)&headers[1],
                    (varying int32*)&headers[2],
                    (varying int32*)&headers[3],
                    (varying int32*)&headers[4]);

        /* Now sum and fold the values */
        unsigned int32 sum = fold(headers[0]);
        sum += fold(headers[1]);
        sum += fold(headers[2]);
        sum += fold(headers[3]);
        sum += fold(headers[4]);
        sum = fold(fold(sum));

        packed_store_active(&checksums[base], (sum == 0xffff ? sum : (0xffff - sum)));
        base += programCount;
    }
}

/*
 * XXX: ISPC doesn't support unions, so we have to manually
 * sort data values into position.
 */
struct ipv4_pseudoheader {
    unsigned int32 src_address;
    unsigned int32 dst_address;
    unsigned int32 proto_len;
};

inline unsigned int16 bswap(unsigned int16 x)
{
    return (x << 8) | (x >> 8);
}

inline void populate_pseudoheaders(const uniform int32 input[],
                                   varying struct ipv4_pseudoheader *phdr)
{
    unsigned int32<5> headers;
    aos_to_soa5((uniform int32*)input,
                (varying int32*)&headers[0],
                (varying int32*)&headers[1],
                (varying int32*)&headers[2],
                (varying int32*)&headers[3],
                (varying int32*)&headers[4]);

    unsigned int16 length = headers[0] >> 16;
    length = bswap(bswap(length) - 20);

    unsigned int32 protocol = (headers[2] >> 8) & 0xff;

    phdr->src_address = headers[3];
    phdr->dst_address = headers[4];
    phdr->proto_len = protocol << 24 | length;
}

export void checksum_ipv4_pseudoheaders(const unsigned int8* const uniform ipv4_header_ptrs[],
                                        uniform unsigned int16 count,
                                        unsigned int32 uniform checksums[])
{
    uniform int32 scratch[5 * programCount];
    uniform int base = 0;

    foreach (i = 0 ... count) {
        /* Copy IPv4 headers into scratch area */
        memcpy(&scratch[5 * programIndex], (unsigned int8* const)ipv4_header_ptrs[i], 20);
        struct ipv4_pseudoheader phdrs;
        populate_pseudoheaders(&scratch[0], &phdrs);

        unsigned int32 sum = fold(phdrs.src_address);
        sum += fold(phdrs.dst_address);
        sum += fold(phdrs.proto_len);
        sum = fold(fold(sum));

        packed_store_active(&checksums[base], sum);
        base += programCount;
    }
}

export uniform unsigned int32 checksum_data_aligned(const uniform unsigned int32 data[],
                                                    uniform unsigned int16 length)
{
    unsigned int32 sum = 0;
    foreach (i = 0 ... length) {
        sum += fold(data[i]);
    }

    return (reduce_add(fold(sum)));
}
