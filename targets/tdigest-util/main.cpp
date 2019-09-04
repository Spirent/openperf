#include "stats/icp_tdigest.h"
#include "stats/icp_tdigest.tcc"
#include <iostream>
#include <iomanip>
#include <algorithm>
#include <type_traits>
#include <random>
#include <string_view>
#include <string.h>
#include <chrono>
#include <unordered_map>

#include <getopt.h>

using namespace icp::stats;

/**
 * Global parameters
 */

static double min_value = 1; // Minimum data value expected.
static double max_value = 100; // Maximum data value expected.
static uint64_t value_count = 1000; // Number of values to insert.
static double compression_factor = 10; // Compression factor supplied by user.
static double max_error = 6.0; // Maximum percent error.
static bool verbose = false;
static bool quiet = false;
static unsigned trial_count = 2;
static std::string value_type = "unsigned long long int"; // Data type to use for t-digest values.
static std::string weight_type = "unsigned long long int"; // Data type to use for t-digest weights.
static std::string input_type = "integral"; // Type to use for generated test data.

static enum class test_mode { COMPRESSION_FACTOR = 0,
                              PERCENT_ERROR,
                              VERIFY,
                              BENCHMARK } mode;

static const std::unordered_map<std::string_view, test_mode> mode_names
    {
     {"compression-factor", test_mode::COMPRESSION_FACTOR},
     {"percent-error", test_mode::PERCENT_ERROR},
     {"verify", test_mode::VERIFY},
     {"benchmark", test_mode::BENCHMARK}
    };

/**
 * Command-line argument handling.
 */

struct cli_option {
    struct option opt;
    std::string_view description;
};

static struct cli_option cli_options[] = {
    {{"min-value", required_argument, 0, 's'}, "minimum expected value"},
    {{"max-value", required_argument, 0, 'S'}, "maximum expected value"},
    {{"value-count", required_argument, 0, 'c'}, "number of values to insert"},
    {{"compression-factor", required_argument, 0, 'f'},
     "compression factor (aka t-digest maximum size)"},
    {{"max-error", required_argument, 0, 'e'},
     "maximum allowable error from calculated CDF, in percent (e.g. 6 -> 6%)"},
    {{"trials", required_argument, 0, 't'},
     "number of trials to execute for above parameters"},
    {{"input-type", required_argument, 0, 'i'},
     "data type of input data (integral or floating_point)"},
    {{"value-type", required_argument, 0, 'a'},
     "c++ data type to use for t-digest values"},
    {{"weight-type", required_argument, 0, 'w'},
     "c++ data type to use for t-digest weights"},
    {{"mode", required_argument, 0, 'm'}, "program operation mode"},
    {{"quiet", no_argument, 0, 'q'}, "quiet mode"},
    {{"verbose", no_argument, 0, 'v'}, "output additional information"},
    {{"help", no_argument, 0, 'h'}, "display this help text"},
    {{0, 0, 0, 0}, ""}};

static void print_usage()
{
    // Brief overview of the program.
    std::cout << std::endl
              << "Utility to assist with t-digest integration." << std::endl
              << "It has four modes of operation:" << std::endl
              << "    1. Suggest a compression factor." << std::endl
              << "    2. Evaluate error percentages." << std::endl
              << "    3. Verify error range for a given maximum percent error." << std::endl
              << "    4. Benchmkark time required to insert values into a t-digest." << std::endl;

    std::cout << std::endl;

    // Now let's tell everyone about our options.
    // How much extra space to insert after the long options.
    static constexpr size_t space_fudge = 3;
    size_t max_len = 0;
    for (auto &opt: cli_options) {
        if (opt.opt.name == nullptr) break;

        max_len = std::max(max_len, strlen(opt.opt.name));
    }

    std::cout << "Usage Info: " << std::endl;

    for (auto &opt: cli_options) {
        if (opt.opt.name == nullptr) break;
        std::cout << "  " << "-" << static_cast<unsigned char>(opt.opt.val) << ",  "
                  << "--" << std::left << std::setw(max_len + space_fudge)
                  << opt.opt.name << " " << opt.description << std::endl;
    }
}

static void set_test_mode(std::string_view mode_arg)
{
    auto mode_val = mode_names.find(mode_arg);

    if (mode_val == mode_names.end()) {
        std::string valid_mode_string;
        std::for_each(mode_names.begin(), mode_names.end(), [&](auto &mode_string){
                                                                valid_mode_string += std::string(mode_string.first)
                                                                    + ", ";});

        valid_mode_string.erase(valid_mode_string.size() - 2, 2);
        std::cerr << "Invalid test mode " << mode_arg << ". "
                  << "Valid test modes include: " << valid_mode_string << std::endl;
        exit(EXIT_FAILURE);
    }

    mode = mode_val->second;
}

static std::string make_shortopts()
{
    std::string to_return;

    for (auto &opt : cli_options) {
        if (opt.opt.name != 0) {
            to_return.push_back(static_cast<char>(opt.opt.val));
            if (opt.opt.has_arg != no_argument) {
                to_return.append(":");
            }
        }
    }

    return (to_return);
}

static void process_options(int argc, char* argv[])
{
    auto short_opts = make_shortopts();

    // Extract the relevant option structs so we can pass them to
    // getopt_long
    std::vector<struct option> options;
    std::transform(std::begin(cli_options), std::end(cli_options),  std::back_inserter(options),
                   [](const struct cli_option &opt) {return opt.opt;} );

    int opt_index = 0;
    while (true)
    {
      int opt =
          getopt_long(argc, argv, short_opts.c_str(), options.data(), &opt_index);

      if (opt == -1) {
          break;
      }
      if (opt == '?') {
          print_usage();
          exit(EXIT_FAILURE);
      } else if (opt == 'h') {
          print_usage();
          exit(EXIT_SUCCESS);
      } else if (opt == 's') {
          min_value = atof(optarg);
      } else if (opt == 'S') {
          max_value = atof(optarg);
      } else if (opt == 't') {
          trial_count = atoi(optarg);
      } else if (opt == 'c') {
          value_count = atoll(optarg);
      } else if (opt == 'f') {
          compression_factor = atoi(optarg);
      } else if (opt == 'e') {
          max_error = atof(optarg);
      } else if (opt == 'v') {
          verbose = true;
      } else if (opt == 'a') {
          value_type = optarg;
      } else if (opt == 'w') {
          weight_type = optarg;
      } else if (opt == 'i') {
          input_type = optarg;
      } else if (opt == 'm') {
          set_test_mode(optarg);
      } else if (opt == 'q') {
          quiet = true;
      } else {
          assert(false);
      }
    }
}

static bool validate_parameters()
{
    bool are_valid = true;
    if (min_value < 0) {
        std::cerr << "Minimum value must be greater than or equal to 0." << std::endl;
        are_valid = false;
    }

    if (max_value < min_value) {
        std::cerr << "Maximum value must be greater than minimum value." << std::endl;
        are_valid = false;
    }

    if (value_count < 0) {
        std::cerr << "Value Count must be greater than 0." << std::endl;
        are_valid = false;
    }

    if (max_error < 0) {
        std::cerr << "Maximum percent error must be greater than 0." << std::endl;
        are_valid = false;
    }

    if (trial_count == 0) {
        std::cerr << "Must provide a non-zero trial count." << std::endl;
        are_valid = false;
    }

    if (quiet && verbose) {
        std::cout << "Warning: specifying quiet and verbose flags will lead to unexpected output." << std::endl;
    }

    return (are_valid);
}

static void print_parameters() {
    std::cout << "Test parameters:" << std::endl << std::endl;

    std::cout << "Test Mode: ";
    switch (mode) {
    case test_mode::BENCHMARK:
        std::cout << "benchmark" << std::endl;;
        break;
    case test_mode::COMPRESSION_FACTOR:
        std::cout << "compression factor" << std::endl;
        break;
    case test_mode::PERCENT_ERROR:
        std::cout << "percent error" << std::endl;
        break;
    case test_mode::VERIFY:
        std::cout << "verify" << std::endl;
        break;
    }

    std::cout << "Minimum expected value: " << min_value << std::endl;
    std::cout << "Maximum expected value: " << max_value << std::endl;
    std::cout << "Number of values to test with: " << value_count << std::endl;
    if (mode != test_mode::COMPRESSION_FACTOR) {
        std::cout << "Compression factor: " << compression_factor << std::endl;
    }
    if (mode != test_mode::PERCENT_ERROR) {
        std::cout << "Maximum percent error: " << max_error << std::endl;
    }
    std::cout << "Number of trials to run: " << trial_count << std::endl;
    std::cout << "Value data type: " << value_type << std::endl;
    std::cout << "Weight data type: " << weight_type << std::endl;
    std::cout << "Input data type: " << input_type << std::endl;

    std::cout << std::endl;
}

/**
 * t-digest support functions.
 *
 */

template<typename Inputs>
static auto generate_inputs()
{
    assert(min_value);
    assert(max_value);
    assert(value_count);

    unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
    std::default_random_engine generator(seed);
    std::vector<Inputs> to_return;

    if constexpr (std::is_integral<Inputs>::value)
    {
        std::uniform_int_distribution<Inputs> distribution(min_value, max_value);
        to_return.reserve(value_count);

        std::generate_n(std::back_inserter(to_return), value_count,
                        [&distribution, &generator](){ return distribution(generator); });
    }

    if constexpr (std::is_floating_point<Inputs>::value) {
      std::uniform_real_distribution<Inputs> distribution(min_value, max_value);
      to_return.reserve(value_count);

      std::generate_n(std::back_inserter(to_return), value_count,
                      [&distribution, &generator]() { return distribution(generator); });
    }

    return (to_return);
}

/**
 * Function to run a single insertion trial and calculate
 * worst percent error from CDF.
 *
 * Returns a std::pair consisting of:
 *  - worst percent error.
 *  - total insertion time.
 */
template<typename Values, typename Weight, typename Inputs>
static auto tdigest_trial(const std::vector<Inputs> &values,
                          size_t comp_factor)
{
    icp_tdigest<Values, Weight> digest(comp_factor);

    auto start = std::chrono::high_resolution_clock::now();
    for (auto i: values) {
        digest.insert(i);
    }

    digest.merge();
    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> diff = end - start;

    double epsilon = -1;

    for (double x = min_value; x <= max_value; x += max_value / 10) {
      double target = x / max_value;
      epsilon = std::max(fabs(target - digest.cumulative_distribution(x)),
                         epsilon);
    }

    return (std::make_pair(epsilon, diff.count()));
}

/**
 * Test case functions. Ultimately the mode parameter maps to
 * one of these functions.
 */

template<typename Values, typename Weight, typename Inputs>
static auto find_compression_factor()
{
    auto inputs = generate_inputs<Inputs>();

    static constexpr unsigned min = 10;
    static constexpr unsigned delta = 2;

    unsigned current = min;

    while (true) {
        auto [epsilon, time] = tdigest_trial<Values, Weight>(inputs, current);

        if (epsilon < (max_error / 100.0)) {
            break;
        }

        current += delta;
    }

    return (current);
}

template<typename Values, typename Weight, typename Inputs>
static auto find_max_error()
{
    auto inputs = generate_inputs<Inputs>();

    auto [epsilon, duration] = tdigest_trial<Values, Weight>(inputs, compression_factor);

    return (epsilon);
}

template<typename Values, typename Weight, typename Inputs>
static auto verify_error_comp_factor()
{
    auto inputs = generate_inputs<Inputs>();

    auto [epsilon, time] =
        tdigest_trial<Values, Weight>(inputs, compression_factor);

    if (epsilon <= (max_error / 100.0)) {
        return (true);
    }

    return (false);
}

template <typename Values, typename Weight, typename Inputs>
static auto benchmark()
{
    auto inputs = generate_inputs<Inputs>();

    auto [epsilon, time] = tdigest_trial<Values, Weight>(inputs, compression_factor);

    if (epsilon <= (max_error / 100.0)) {
        return (time);
    }

    return (-1.0);
}

/**
 * Function to invoke the requested test now that we've resolved our three
 * template parameters.
 */
template <typename Values, typename Weight, typename Inputs>
static double run_test_impl() {
    switch (mode) {
    case test_mode::PERCENT_ERROR:
        return (find_max_error<Values, Weight, Inputs>());
        break;
    case test_mode::COMPRESSION_FACTOR:
        return (find_compression_factor<Values, Weight, Inputs>());
        break;
    case test_mode::VERIFY:
        return (verify_error_comp_factor<Values, Weight, Inputs>());
        break;
    case test_mode::BENCHMARK:
        return (benchmark<Values, Weight, Inputs>());
        break;
  }

    throw std::invalid_argument("Invalid test mode");
}

/**
 * Function to bind the input type parameter when starting a test.
 */
template <typename V, typename W>
static auto bind_input() {
    if (input_type == "integral") {
        return (run_test_impl<V, W, uint64_t>());
    }
    if (input_type == "floating_point") {
        return (run_test_impl<V, W, double>());
    }

    std::cout << "Unsupported input type " << input_type << ". "
              << "Supported value types include: integral, floating_point" << std::endl;

    exit(EXIT_FAILURE);
}

/**
 * Bind the weight type parameter and call function to bind input type parameter.
 * Used when starting a test.
 */
template <typename V>
static auto bind_weight() {
    std::string weight_types;
#define WEIGHT_TYPE(type_)                                              \
    weight_types = weight_types + #type_ + ", ";                        \
    if (weight_type == #type_) {                                        \
        return (bind_input<V, type_>());                                \
    }
    WEIGHT_TYPE(unsigned long long int)
    WEIGHT_TYPE(double)
#undef WEIGHT_TYPE

    // Trim the trailing comma
    weight_types.erase(weight_types.size() - 2, 2);

    std::cout << "Unsupported weight type " << weight_type << ". "
              << "Supported value types include: " << weight_types << std::endl;

    exit(EXIT_FAILURE);
}

/**
 * aka bind_value();
 * Bind the value type template parameter and call function to bind weight type parameter.
 * Used when starting a test.
 */
static auto run_test() {
    std::string value_types;
#define VALUE_TYPE(type_)                                               \
    value_types = value_types + #type_ + ", ";                          \
    if (value_type == #type_) {                                         \
        return (bind_weight<type_>());                                  \
    }
    VALUE_TYPE(float)
    VALUE_TYPE(double)
    VALUE_TYPE(long double)
    VALUE_TYPE(short int)
    VALUE_TYPE(unsigned short int)
    VALUE_TYPE(int)
    VALUE_TYPE(unsigned int)
    VALUE_TYPE(long int)
    VALUE_TYPE(unsigned long int)
    VALUE_TYPE(long long int)
    VALUE_TYPE(unsigned long long int)
#undef VALUE_TYPE

    // Trim the trailing comma.
    value_types.erase(value_types.size() - 2, 2);

    std::cout << "Unsupported value type " << value_type << ". "
              << "Supported value types include: " << value_types << std::endl;

    exit(EXIT_FAILURE);
}

static void output_results(std::vector<double> &results)
{
  auto [min, max] = std::minmax_element(results.begin(), results.end());

  auto sum = std::accumulate(results.begin(), results.end(), 0.0);

  auto avg = sum / static_cast<double>(results.size());

  if (verbose) {
    std::cout << "Individual Trial Results:" << std::endl;
    for (size_t i = 0; i < results.size(); i++) {
      std::cout << "[" << std::setw(3) << i + 1 << "]  " << results.at(i)
                << std::endl;
    }
    std::cout << std::endl;
  }

  switch (mode) {
  case test_mode::BENCHMARK:
      {
      // Benchmark results can have out-of-range percent error values.
      // benchmkark() returns -1 in those cases.
      // Check for cases of -1 and compute the benchmark values for the
      // valid results only and warn the user.
      auto invalid_trials = std::count_if(results.begin(), results.end(), [](double val) {return val < 0;});
      if (invalid_trials != 0) {
          std::cout << "Warning: detected " << invalid_trials << " trials with percent error greater than "
                    << max_error << ". Results below are only for valid trials." << std::endl;

          sum = std::accumulate(results.begin(), results.end(), 0.0, [](double sum, double val) {
                                                                         if (val >= 0) {
                                                                             sum += val;
                                                                         }
                                                                         return (sum);});
          avg = (results.size() - invalid_trials != 0) ? sum /
              static_cast<double>(results.size() - invalid_trials) : 0;

          min = std::min_element(results.begin(), results.end(),
                                 [](double &lhs, double &rhs){
                                     if (lhs < 0) return (false);
                                     else return (lhs < rhs); });
      }

      if (!quiet) {
        std::cout << "Time to insert " << value_count
                  << " values in the range [" << min_value << ", " << max_value
                  << "]:" << std::endl;
      }
      std::cout << "Benchmark results min/avg/max " << *min << "/" << avg << "/"
                << *max << "  ms" << std::endl;
      break;
  }
  case test_mode::COMPRESSION_FACTOR:
      if (!quiet) {
          std::cout << "For " << value_count << " values in the range [" << min_value << ", " << max_value
                    << "]:" << std::endl
                    << "For a percent error of not more than " << max_error
                    << " percent, recommended compression factor: " << *max << std::endl;
      }
      std::cout << "Scale factor results min/avg/max  " << *min << "/" << avg << "/" << *max << std::endl;

      break;
  case test_mode::PERCENT_ERROR:
      if (!quiet) {
          std::cout << "For " << value_count << " values in the range ["
                    << min_value << ", " << max_value << "], "
                    << "with a compression factor of " << compression_factor << ":"
                    << std::endl;
      }
      std::cout << "Percent error results min/avg/max  " << (*min) * 100 << "/" << avg * 100
                << "/" << (*max) * 100 << " percent" << std::endl;
      break;
  case test_mode::VERIFY:
      if (!quiet) {
          std::cout << "For " << value_count << " values in the range ["
                    << min_value << ", " << max_value << "], "
                    << "with a compression factor of " << compression_factor
                    << " and a maximum percent error of " << max_error << ": "
                    << std::endl;
      }
      std::cout << "Percent of trials that satisfied constraints: "
                << sum / results.size() * 100 << " percent" << std::endl;
      break;
  }

}

int main (int argc, char* argv[]) {

    process_options(argc, argv);
    if (!validate_parameters()) {
        exit(EXIT_FAILURE);
    }

    if (!quiet) {
        print_parameters();
    }

    std::vector<double> results;
    for (unsigned i = 0; i < trial_count; i++) {
        if (verbose) { std::cout << "***Running Trial: " << i << "***" << std::endl; }

        results.push_back(run_test());
    }

    output_results(results);

    return (0);
}
