#include "inet.h"
#include "signature_scramble.h"

static const int is_valid = (1 << 2);

export uniform unsigned int16
decode_signatures(const unsigned int8* const uniform payloads[],
                  uniform unsigned int16 count,
                  unsigned int32 uniform stream_ids[],
                  unsigned int32 uniform sequence_numbers[],
                  unsigned int32 uniform timestamps_hi[],
                  unsigned int32 uniform timestamps_lo[],
                  int32 uniform flags[])
{
    uniform int scratch[4 * programCount];

    /* We treat the signature as 4 x 32 bit integers */
    unsigned int<4> data;
    unsigned int<4> mask;
    unsigned int<4> signature;

    unsigned int nb_sigs = 0;

    foreach(i = 0 ... count)
    {
        /* Copy the payloads into a linear array  */
        memcpy(
            &scratch[programIndex * 4], (unsigned int8* const)payloads[i], 16);

        /* and transform the array data into a SIMD friendly format */
        aos_to_soa4(&scratch[0],
                    (varying int*)&data[0],
                    (varying int*)&data[1],
                    (varying int*)&data[2],
                    (varying int*)&data[3]);

        /* Descramble the signature data */
        unsigned int8 scramble_key = ~(data[0] & 0xff);
        get_scramble_mask(scramble_key ^ 0xff, mask);
        signature[0] = ntohl(data[0]) ^ mask[0];
        signature[1] = ntohl(data[1]) ^ mask[1];
        signature[2] = ntohl(data[2]) ^ mask[2];
        signature[3] = ntohl(data[3]) ^ mask[3];

        /* Verify that the scramble_key and byte10 are complements */
        unsigned int8 byte10 = (signature[2] >> 8) & 0xff;

        /* Perform enhanced detection check */
        unsigned int16 edm1 = ~(signature[1] >> 8) & 0xffff;
        unsigned int16 edm2 = (signature[1] << 8 | signature[2] >> 24) & 0xffff;

        /* Determine which signatures are valid */
        bool valid_sig = and(scramble_key == byte10, edm1 == edm2);

        /* Copy data out; mark valid data as such */
        /* Valid signature found; copy data out */
        stream_ids[i] = signature[0] << 8 | signature[1] >> 24;
        sequence_numbers[i] = signature[1] << 24 | signature[2] >> 8;
        timestamps_lo[i] = signature[2] << 24 | signature[3] >> 8;
        timestamps_hi[i] = (signature[3] >> 2) & 0x3f;
        flags[i] = valid_sig ? is_valid | (signature[3] & 0x3) : 0;

        /* Keep track of the number of valid signatures */
        nb_sigs += valid_sig;
    }

    return (reduce_add(nb_sigs));
}
