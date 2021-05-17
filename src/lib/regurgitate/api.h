#ifndef _LIB_REGURGITATE_API_H_
#define _LIB_REGURGITATE_API_H_

#include <stdio.h>

#ifdef __cplusplus
constexpr unsigned regurgitate_optimum_length = 256;
extern "C" {
#endif

/*
 * Initialiation, i.e. run-time benchmarking function.
 * Initialization occurs only once, regardless of how many times
 * this function is called.
 */
void regurgitate_init();

/**
 * Log library details to the specified file.
 *
 * @param[in] output
 *  stream to write data to
 */
void regurgitate_log_info(FILE* output);

/*
 * Merge functions
 */

/**
 * Perform a t-digest merge on the input keys/values and
 * compress them into the output keys/values.
 *
 * @param[in] in_keys[]
 *  array of input keys
 * @param[in] in_values[]
 *  array of input values
 * @param[in] in_length
 *  length of input arrays
 * @param[in,out] out_keys[]
 *  array of output keys
 * @param[in,out] out_vals[]
 *  array of output values
 * @param[in] out_length
 *  length of output arrays
 *
 * @return
 *  the number of values written to the output arrays
 */
unsigned regurgitate_merge_float_float(float in_keys[],
                                       float in_vals[],
                                       unsigned in_length,
                                       float out_keys[],
                                       float out_vals[],
                                       unsigned out_length);

unsigned regurgitate_merge_float_double(float in_keys[],
                                        double in_vals[],
                                        unsigned in_length,
                                        float out_keys[],
                                        double out_vals[],
                                        unsigned out_length);

/*
 * Sort functions
 */

/**
 * Sort the keys/values in place. The scratch arrays must be as least
 * as long as the key/value arrays.
 *
 * @param[in,out] keys
 *  array of keys
 * @param[in,out] vals
 *  array of values
 * @param[in] length
 *  length of key and value arrays
 * @param[in] scratch_k
 *  scratch area for key sorting
 * @param[in] scratch_w
 *  scratch area for value sorting
 */
void regurgitate_sort_float_float(float keys[],
                                  float vals[],
                                  unsigned length,
                                  float scratch_k[],
                                  float scratch_v[]);

void regurgitate_sort_float_double(float keys[],
                                   double vals[],
                                   unsigned length,
                                   float scratch_k[],
                                   double scratch_v[]);

#ifdef __cplusplus
}
#endif

#endif /* _LIB_REGURGITATE_API_H_ */
