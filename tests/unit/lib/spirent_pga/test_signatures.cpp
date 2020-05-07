#include <vector>

#include "catch.hpp"

#include "spirent_pga/api.h"
#include "api_test.h"

struct spirent_signature
{
    uint8_t data[16];
    uint16_t crc;
    uint16_t cheater;
} __attribute__((packed));

TEST_CASE("signature functions", "[spirent-pga]")
{
    /* Common reference data for all tests */
    constexpr uint16_t nb_signatures = 128;
    constexpr uint16_t nb_streams = 8;
    constexpr uint64_t timestamp = 0x7d004000;
    constexpr int flag_value = 0x1; /* last timestamp set */

    /*
     * This wacky signature array two step is due to our API.
     * We expect signatures to be written to packet buffers specified
     * by pointers.
     */
    std::vector<spirent_signature> ref_signatures(nb_signatures);
    std::vector<uint8_t*> ref_signature_ptrs(nb_signatures);
    std::vector<uint32_t> stream_ids(nb_signatures);
    std::vector<uint32_t> sequence_nums(nb_signatures);
    std::vector<uint32_t> timestamps_hi(nb_signatures);
    std::vector<uint32_t> timestamps_lo(nb_signatures);
    std::vector<int> flags(nb_signatures, flag_value);

    for (auto i = 0; i < nb_signatures; i++) {
        ref_signatures[i] = {{}, 0, 0};
        ref_signature_ptrs[i] = &ref_signatures[i].data[0];
        stream_ids[i] = i % nb_streams;
        sequence_nums[i] = i / nb_streams;
        timestamps_hi[i] = timestamp >> 32;
        timestamps_lo[i] = (timestamp & 0xffffffff) + i;
    }

    SECTION("implementations")
    {
        auto& functions = pga::functions::instance();

        /*
         * Start by generating scalar signatures.  We use them as the
         * reference for the vector implementations.
         */
        auto scalar_fn =
            pga::test::get_function(functions.encode_signatures_impl,
                                    pga::instruction_set::type::SCALAR);
        REQUIRE(scalar_fn != nullptr);

        scalar_fn(ref_signature_ptrs.data(),
                  stream_ids.data(),
                  sequence_nums.data(),
                  timestamps_hi.data(),
                  timestamps_lo.data(),
                  flags.data(),
                  ref_signatures.size());

        SECTION("encoding")
        {
            unsigned vector_tests = 0;

            for (auto instruction_set : pga::test::vector_instruction_sets()) {
                auto vector_fn = pga::test::get_function(
                    functions.encode_signatures_impl, instruction_set);
                /*
                 * Obviously, we can't test a null function or an instruction
                 * set our CPU won't run.
                 */
                if (!(vector_fn
                      && pga::instruction_set::available(instruction_set))) {
                    continue;
                }

                INFO("instruction set = "
                     << pga::instruction_set::to_string(instruction_set));

                vector_tests++;

                std::vector<spirent_signature> vec_signatures(nb_signatures);
                std::vector<uint8_t*> vec_signature_ptrs(nb_signatures);

                for (auto i = 0; i < nb_signatures; i++) {
                    vec_signatures[i] = {{}, 0, 0};
                    vec_signature_ptrs[i] = &vec_signatures[i].data[0];
                }

                vector_fn(vec_signature_ptrs.data(),
                          stream_ids.data(),
                          sequence_nums.data(),
                          timestamps_hi.data(),
                          timestamps_lo.data(),
                          flags.data(),
                          vec_signatures.size());

                /* Now, compare all signature data and the cheater field */
                for (auto i = 0; i < nb_signatures; i++) {
                    REQUIRE(std::equal(ref_signatures[i].data,
                                       ref_signatures[i].data + 16,
                                       vec_signatures[i].data));
                }
            }
            REQUIRE(vector_tests > 0);
        }

        SECTION("decoding")
        {
            /*
             * All of our decoding functions should return the proper
             * sequence numbers, stream ids, and timestamps.
             */
            unsigned decode_tests = 0;

            for (auto instruction_set : pga::test::instruction_sets()) {
                auto decode_fn = pga::test::get_function(
                    functions.decode_signatures_impl, instruction_set);

                if (!(decode_fn
                      && pga::instruction_set::available(instruction_set))) {
                    continue;
                }

                INFO("instruction set = "
                     << pga::instruction_set::to_string(instruction_set));

                decode_tests++;

                std::vector<uint32_t> out_stream_ids(nb_signatures);
                std::vector<uint32_t> out_sequence_nums(nb_signatures);
                std::vector<uint32_t> out_timestamps_hi(nb_signatures);
                std::vector<uint32_t> out_timestamps_lo(nb_signatures);
                std::vector<int> out_flags(nb_signatures);

                auto count = decode_fn(ref_signature_ptrs.data(),
                                       ref_signature_ptrs.size(),
                                       out_stream_ids.data(),
                                       out_sequence_nums.data(),
                                       out_timestamps_hi.data(),
                                       out_timestamps_lo.data(),
                                       out_flags.data());

                /* Did we find them all? */
                REQUIRE(count == nb_signatures);

                /* Verify decoded signature data */
                REQUIRE(std::equal(std::begin(stream_ids),
                                   std::end(stream_ids),
                                   std::begin(out_stream_ids)));
                REQUIRE(std::equal(std::begin(sequence_nums),
                                   std::end(sequence_nums),
                                   std::begin(out_sequence_nums)));
                REQUIRE(std::equal(std::begin(timestamps_hi),
                                   std::end(timestamps_hi),
                                   std::begin(out_timestamps_hi)));
                REQUIRE(std::equal(std::begin(timestamps_lo),
                                   std::end(timestamps_lo),
                                   std::begin(out_timestamps_lo)));

                /* All flag values should be valid */
                REQUIRE(std::all_of(std::begin(out_flags),
                                    std::end(out_flags),
                                    [&](const auto& x) {
                                        return (pga_status_flag(x)
                                                == pga_signature_status::valid);
                                    }));
            }
            REQUIRE(decode_tests > 1);
        }
    }

    SECTION("API")
    {

        /* Clear any signature data */
        std::generate(
            std::begin(ref_signatures), std::end(ref_signatures), []() {
                return (spirent_signature{{}, 0, 0});
            });

        /* Generate a unified timestamp vector */
        std::vector<uint64_t> timestamps{};
        std::transform(std::begin(timestamps_hi),
                       std::end(timestamps_hi),
                       std::begin(timestamps_lo),
                       std::back_inserter(timestamps),
                       [](const auto& hi, const auto& lo) {
                           return (static_cast<uint64_t>(hi) << 32 | lo);
                       });

        pga_signatures_encode(ref_signature_ptrs.data(),
                              stream_ids.data(),
                              sequence_nums.data(),
                              timestamps.data(),
                              flags.data(),
                              ref_signature_ptrs.size());

        /*
         * Verify that every signature has a non-zero CRC value and
         * zeroed out cheater value.
         */
        std::for_each(std::begin(ref_signatures),
                      std::end(ref_signatures),
                      [](auto& sig) {
                          REQUIRE(sig.crc != 0);
                          REQUIRE(sig.cheater == 0);
                      });

        /* Make sure the presence of the CRC doesn't impede decoding */
        std::vector<uint32_t> out_stream_ids(nb_signatures);
        std::vector<uint32_t> out_sequence_nums(nb_signatures);
        std::vector<uint64_t> out_timestamps(nb_signatures);
        std::vector<int> out_flags(nb_signatures);

        /* XXX: why is this cast needed at all?!?! */
        auto count =
            pga_signatures_decode((const uint8_t**)ref_signature_ptrs.data(),
                                  ref_signature_ptrs.size(),
                                  out_stream_ids.data(),
                                  out_sequence_nums.data(),
                                  out_timestamps.data(),
                                  out_flags.data());
        REQUIRE(count == nb_signatures);
    }
}
