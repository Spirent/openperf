
# Dynamic Results Framework

Dynamic Results Framework is located at `src/framework/dynamic` path. Framework uses `digestible` as external dependency. Digestible is located at `deps/digestible` path.

## Using

To embed Dynamic Results to own generator include `framework/dynamic/spool.hpp` and use *spool* template to create and control the collection of Dynamic Results. Also, you may want include `framework/dynamic/api.h` to get access to API structures and supporting conversion functions of `openperf::dynamic::configuration` and `openperf::dynamic::results`.

## Spool

```cpp
template <typename T> class spool
```

The spool is a collection of the dynamic resutls objects, such as Threshods and T-Digests. You should provide *extractor* function for the spool constructor. The template parameter *T* is a struct of the statistics data.

### Configuring

```cpp
spool::configure(const configuration&, const T& initial_stat = {});
```

Use `configure()` method to configure dynamic results. First parameter is a dynamic results configuration object. Second optional parameter *initial_stat* of type *T* can be used for assigning initial value of the statistics used for calculation of delta values for fields.

*IMPORTANT:* Spool uses *initial_stat* for checking fields availability during configuration process. Spool applyes *extrator* function to *initial_stat* value. Hence, if statistics type *T* is a complex type, contained nested dynamically-sized data containers, then you ***should*** provide the *initial_stat* value with initialized sizes.

### Using
* Use `spool::add(const T& statistics)` method to add statistics data point to spool.
* Use `spool::reset()` method to reset all dynamic results data.
* Use `spool::result()` method to get dynamic results.

## Extractor function

```cpp
std::optional<double> extractor(const T&, std::string_view)
```

Extractor function is a function that extract field from some statistics structure as double value. One accepts as parameter statistics object and string field name. Function returns `std::optional<double>` and should return `std::nullopt` for unknown fields. Function should return valid `double` value for all allowed fields.
