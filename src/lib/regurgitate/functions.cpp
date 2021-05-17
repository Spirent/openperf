#include <random>

#include "functions.hpp"

namespace regurgitate {

template <typename Iterator> void fill_random(Iterator first, Iterator last)
{
    auto device = std::random_device{};
    auto generator = std::mt19937(device());
    auto dist = std::uniform_real_distribution(0.0, 1000000.0);

    while (first != last) { *first++ = dist(generator); }
}

template <typename KeyType, typename ValType, typename Wrapper>
void initialize_sort(Wrapper& wrapper)
{
    using key_array = std::array<KeyType, merge_sort_input_max>;
    using val_array = std::array<ValType, merge_sort_input_max>;

    key_array means, scratch_m;
    val_array weights, scratch_w;

    fill_random(std::begin(means), std::end(means));
    std::iota(std::begin(weights), std::end(weights), 1);

    wrapper.init(means.data(),
                 weights.data(),
                 merge_sort_input_max,
                 scratch_m.data(),
                 scratch_w.data());
}

template <typename KeyType, typename ValType, typename Wrapper>
void initialize_merge(Wrapper& wrapper)
{
    static const size_t output_size = 16;

    auto input_means = std::array<KeyType, merge_sort_input_max>{};
    fill_random(std::begin(input_means), std::end(input_means));

    auto input_weights = std::array<ValType, merge_sort_input_max>{};
    std::iota(std::begin(input_weights), std::end(input_weights), 1);

    auto output_means = std::array<KeyType, output_size>{};
    auto output_weights = std::array<ValType, output_size>{};

    wrapper.init(input_means.data(),
                 input_weights.data(),
                 merge_sort_input_max,
                 output_means.data(),
                 output_weights.data(),
                 output_size);
}

functions::functions()
{
    initialize_sort<float, float>(sort_float_float_impl);
    initialize_sort<float, double>(sort_float_double_impl);

    initialize_merge<float, float>(merge_float_float_impl);
    initialize_merge<float, double>(merge_float_double_impl);
}

} // namespace regurgitate
