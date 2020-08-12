#include "aos_soa_5.h"
#include "inet.h"

inline static unsigned int32 fold(unsigned int32 x)
{
    return ((x >> 16) + (x & 0xffff));
}

inline static uniform unsigned int32 fold(uniform unsigned int32 x)
{
    return ((x >> 16) + (x & 0xffff));
}

/*
 * Instead of shuffling data around and checksumming multiple headers
 * in parallel, we use the SIMD registers to go wide and minimize
 * the number of instructions per header.  This ends up being faster
 * when we're just summing data across the lanes.
 *
 * The masks are used to exclude data from the checksumming process.
 * Additionally, we use the checksum output array as a scratch area
 * to store a temporary sum. We then iterate through the checksum array
 * and calculate the final results in parallel.
 */

#if TARGET_WIDTH == 4

export void checksum_ipv4_headers(const unsigned int8* const uniform ipv4_header_ptrs[],
                                  uniform unsigned int16 count,
                                  unsigned int32 uniform checksums[])
{
    static const unsigned int32 masks[16][3] = {
        { { -1,  0,  0,  0 }, {  0,  0,  0,  0}, {  0,  0,  0,  0} },
        { { -1,  0,  0,  0 }, {  0,  0,  0,  0}, {  0,  0,  0,  0} },
        { { -1,  0,  0,  0 }, {  0,  0,  0,  0}, {  0,  0,  0,  0} },
        { { -1,  0,  0,  0 }, {  0,  0,  0,  0}, {  0,  0,  0,  0} },
        { { -1,  0,  0,  0 }, {  0,  0,  0,  0}, {  0,  0,  0,  0} },
        { { -1,  0,  0,  0 }, {  0,  0,  0,  0}, {  0,  0,  0,  0} },
        { { -1, -1,  0,  0 }, {  0,  0,  0,  0}, {  0,  0,  0,  0} },
        { { -1, -1, -1,  0 }, {  0,  0,  0,  0}, {  0,  0,  0,  0} },
        { { -1, -1, -1, -1 }, {  0,  0,  0,  0}, {  0,  0,  0,  0} },
        { { -1, -1, -1, -1 }, { -1,  0,  0,  0}, {  0,  0,  0,  0} },
        { { -1, -1, -1, -1 }, { -1, -1,  0,  0}, {  0,  0,  0,  0} },
        { { -1, -1, -1, -1 }, { -1, -1, -1,  0}, {  0,  0,  0,  0} },
        { { -1, -1, -1, -1 }, { -1, -1, -1, -1}, {  0,  0,  0,  0} },
        { { -1, -1, -1, -1 }, { -1, -1, -1, -1}, { -1,  0,  0,  0} },
        { { -1, -1, -1, -1 }, { -1, -1, -1, -1}, { -1, -1,  0,  0} },
        { { -1, -1, -1, -1 }, { -1, -1, -1, -1}, { -1, -1, -1,  0} },
    };

    for (uniform int i = 0; i < count; i++) {
        /* XXX: uniform key word required for optimal 4-wide load */
        varying unsigned int32* uniform ptr = (varying unsigned int32*)ipv4_header_ptrs[i];
        uniform int ihl = extract(ptr[0], 0) & 0xf;

        unsigned int32 sum = fold(ptr[0]);
        sum += fold(ptr[1] & masks[ihl][0]);

        if (ihl > 8) {
            sum += fold(ptr[2] & masks[ihl][1]);

            if (ihl > 12) {
                sum += fold(ptr[3] & masks[ihl][2]);
            }
        }

        checksums[i] = reduce_add(sum);
    }

    foreach(i = 0 ... count) {
        unsigned int32 sum = fold(fold(checksums[i]));
        checksums[i] = select(sum == 0xffff, (int32)sum, (int32)(0xffff - sum));
    }
}

#elif TARGET_WIDTH == 8

export void checksum_ipv4_headers(const unsigned int8* const uniform ipv4_header_ptrs[],
                                  uniform unsigned int16 count,
                                  unsigned int32 uniform checksums[])
{
    static const unsigned int32 masks[16][2] = {
        { { -1, -1, -1, -1, -1,  0,  0,  0 }, {  0,  0,  0,  0,  0,  0,  0,  0 } },
        { { -1, -1, -1, -1, -1,  0,  0,  0 }, {  0,  0,  0,  0,  0,  0,  0,  0 } },
        { { -1, -1, -1, -1, -1,  0,  0,  0 }, {  0,  0,  0,  0,  0,  0,  0,  0 } },
        { { -1, -1, -1, -1, -1,  0,  0,  0 }, {  0,  0,  0,  0,  0,  0,  0,  0 } },
        { { -1, -1, -1, -1, -1,  0,  0,  0 }, {  0,  0,  0,  0,  0,  0,  0,  0 } },
        { { -1, -1, -1, -1, -1,  0,  0,  0 }, {  0,  0,  0,  0,  0,  0,  0,  0 } },
        { { -1, -1, -1, -1, -1, -1,  0,  0 }, {  0,  0,  0,  0,  0,  0,  0,  0 } },
        { { -1, -1, -1, -1, -1, -1, -1,  0 }, {  0,  0,  0,  0,  0,  0,  0,  0 } },
        { { -1, -1, -1, -1, -1, -1, -1, -1 }, {  0,  0,  0,  0,  0,  0,  0,  0 } },
        { { -1, -1, -1, -1, -1, -1, -1, -1 }, { -1,  0,  0,  0,  0,  0,  0,  0 } },
        { { -1, -1, -1, -1, -1, -1, -1, -1 }, { -1, -1,  0,  0,  0,  0,  0,  0 } },
        { { -1, -1, -1, -1, -1, -1, -1, -1 }, { -1, -1, -1,  0,  0,  0,  0,  0 } },
        { { -1, -1, -1, -1, -1, -1, -1, -1 }, { -1, -1, -1, -1,  0,  0,  0,  0 } },
        { { -1, -1, -1, -1, -1, -1, -1, -1 }, { -1, -1, -1, -1, -1,  0,  0,  0 } },
        { { -1, -1, -1, -1, -1, -1, -1, -1 }, { -1, -1, -1, -1, -1, -1,  0,  0 } },
        { { -1, -1, -1, -1, -1, -1, -1, -1 }, { -1, -1, -1, -1, -1, -1, -1,  0 } },
    };

    for (uniform int i = 0; i < count; i++) {
        varying unsigned int32* ptr = (varying unsigned int32*)ipv4_header_ptrs[i];
        uniform int ihl = extract(ptr[0], 0) & 0xf;

        unsigned int32 sum = fold(ptr[0] & masks[ihl][0]);

        if (ihl > 8) {
            sum += fold(ptr[1] & masks[ihl][1]);
        }

        checksums[i] = reduce_add(sum);
    }

    foreach(i = 0 ... count) {
        unsigned int32 sum = fold(fold(checksums[i]));
        checksums[i] = select(sum == 0xffff, (int32)sum, (int32)(0xffff - sum));
    }
}

#elif TARGET_WIDTH == 16

export void checksum_ipv4_headers(const unsigned int8* const uniform ipv4_header_ptrs[],
                                  uniform unsigned int16 count,
                                  unsigned int32 uniform checksums[])
{
    static const unsigned int32 masks[16] = {
        { -1, -1, -1, -1, -1,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0 },
        { -1, -1, -1, -1, -1,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0 },
        { -1, -1, -1, -1, -1,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0 },
        { -1, -1, -1, -1, -1,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0 },
        { -1, -1, -1, -1, -1,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0 },
        { -1, -1, -1, -1, -1,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0 },
        { -1, -1, -1, -1, -1, -1,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0 },
        { -1, -1, -1, -1, -1, -1, -1,  0,  0,  0,  0,  0,  0,  0,  0,  0 },
        { -1, -1, -1, -1, -1, -1, -1, -1,  0,  0,  0,  0,  0,  0,  0,  0 },
        { -1, -1, -1, -1, -1, -1, -1, -1, -1,  0,  0,  0,  0,  0,  0,  0 },
        { -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,  0,  0,  0,  0,  0,  0 },
        { -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,  0,  0,  0,  0,  0 },
        { -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,  0,  0,  0,  0 },
        { -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,  0,  0,  0 },
        { -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,  0,  0 },
        { -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,  0 },
    };

    for (uniform int i = 0; i < count; i++) {
        varying unsigned int32* ptr = (varying unsigned int32*)ipv4_header_ptrs[i];
        uniform int ihl = extract(ptr[0], 0) & 0xf;
        checksums[i] = reduce_add(fold(ptr[0] & masks[ihl]));
    }

    foreach(i = 0 ... count) {
        unsigned int32 sum = fold(fold(checksums[i]));
        checksums[i] = select(sum == 0xffff, (int32)sum, (int32)(0xffff - sum));
    }
}

#endif

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

    unsigned int length = headers[0] >> 16;
    unsigned int ihl = headers[0] & 0xf;
    length = bswap(bswap(length) - (4 * ihl));

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

    foreach (i = 0 ... count) {
        /* Copy IPv4 headers into scratch area */
        memcpy(&scratch[5 * programIndex], (unsigned int8* const)ipv4_header_ptrs[i], 20);
        struct ipv4_pseudoheader phdrs;
        populate_pseudoheaders(&scratch[0], &phdrs);

        unsigned int32 sum = fold(phdrs.src_address);
        sum += fold(phdrs.dst_address);
        sum += fold(phdrs.proto_len);
        sum = fold(fold(sum));

        packed_store_active(&checksums[extract(i, 0)], sum);
    }
}

struct ipv6_header {
    unsigned int32 version_class_flow;
    unsigned int16 payload_length;
    unsigned int8 protocol;
    unsigned int8 hop_limit;
    unsigned int8 src_address[16];
    unsigned int8 dst_address[16];
};

export void checksum_ipv6_pseudoheaders(const unsigned int8* const uniform ipv6_header_ptr[],
                                        uniform unsigned int16 count,
                                        unsigned int32 uniform checksums[])
{
    /*
     * Transposing the IPv6 header into lanes is expensive, so this function
     * only sums the address data in parallel.
     */
    for (uniform int i = 0; i < count; i++) {
        const ipv6_header* uniform hdr =  (const ipv6_header* uniform)ipv6_header_ptr[i];

        uniform unsigned int32 sum = fold(hdr->payload_length) + fold(ntohl(hdr->protocol));

        /* Treat the IPv6 header as an array of 32 bit data */
        const unsigned int32* uniform data = (const unsigned int32* uniform)(&hdr->src_address);

        /* Sum the IPv6 src/dst values */
        unsigned int32 tmp = 0;
        foreach (j = 0 ... 8) {
            tmp += fold(data[j]);
        }

        /* Add, reduce, and fold the checksum into place */
        checksums[i] = fold(reduce_add(fold(tmp)) + fold(sum));
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
