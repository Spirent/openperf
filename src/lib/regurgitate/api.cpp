#include "api.h"
#include "functions.hpp"
#include "instruction_set.hpp"

static void regurgitate_log_implementation_info(FILE* output,
                                                std::string_view function,
                                                std::string_view instructions)
{
    fprintf(output,
            "regurgitate: using %.*s instructions for %.*s\n",
            static_cast<int>(instructions.size()),
            instructions.data(),
            static_cast<int>(function.size()),
            function.data());
}

extern "C" {

void regurgitate_init()
{
    [[maybe_unused]] auto& functions = regurgitate::functions::instance();
}

void regurgitate_log_info(FILE* output)
{
    auto& functions = regurgitate::functions::instance();

    regurgitate_log_implementation_info(
        output,
        functions.merge_float_float_impl.name,
        regurgitate::instruction_set::to_string(
            regurgitate::get_instruction_set(
                functions.merge_float_float_impl)));
    regurgitate_log_implementation_info(
        output,
        functions.merge_float_double_impl.name,
        regurgitate::instruction_set::to_string(
            regurgitate::get_instruction_set(
                functions.merge_float_float_impl)));
    regurgitate_log_implementation_info(
        output,
        functions.sort_float_float_impl.name,
        regurgitate::instruction_set::to_string(
            regurgitate::get_instruction_set(functions.sort_float_float_impl)));
    regurgitate_log_implementation_info(
        output,
        functions.sort_float_double_impl.name,
        regurgitate::instruction_set::to_string(
            regurgitate::get_instruction_set(functions.sort_float_float_impl)));
}

/*
 * Merge functions
 */
unsigned regurgitate_merge_float_float(float in_keys[],
                                       float in_vals[],
                                       unsigned in_length,
                                       float out_keys[],
                                       float out_vals[],
                                       unsigned out_length)
{
    auto& functions = regurgitate::functions::instance();
    return (functions.merge_float_float_impl(
        in_keys, in_vals, in_length, out_keys, out_vals, out_length));
}

unsigned regurgitate_merge_float_double(float in_keys[],
                                        double in_vals[],
                                        unsigned in_length,
                                        float out_keys[],
                                        double out_vals[],
                                        unsigned out_length)
{
    auto& functions = regurgitate::functions::instance();
    return (functions.merge_float_double_impl(
        in_keys, in_vals, in_length, out_keys, out_vals, out_length));
}

/*
 * Sort functions
 */
void regurgitate_sort_float_float(float in_keys[],
                                  float in_vals[],
                                  unsigned length,
                                  float scratch_k[],
                                  float scratch_v[])
{
    auto& functions = regurgitate::functions::instance();
    functions.sort_float_float_impl(
        in_keys, in_vals, length, scratch_k, scratch_v);
}

void regurgitate_sort_float_double(float in_keys[],
                                   double in_vals[],
                                   unsigned length,
                                   float scratch_k[],
                                   double scratch_v[])
{
    auto& functions = regurgitate::functions::instance();
    functions.sort_float_double_impl(
        in_keys, in_vals, length, scratch_k, scratch_v);
}
}
