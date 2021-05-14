#ifndef _LIB_REGURGITATE_HPP_
#define _LIB_REGURGITATE_HPP_

#include <algorithm>
#include <array>
#include <numeric>

#include <iterator>
#include <iostream>

#include "functions.hpp"

namespace ispc {

/*
 * Generate a SFINAE wrapper for the specified key, value pairs.
 */
#define ISPC_MERGE_AND_SORT_WRAPPER(key_type, val_type)                        \
    template <typename K, typename V>                                          \
    inline std::enable_if_t<std::conjunction_v<std::is_same<K, key_type>,      \
                                               std::is_same<V, val_type>>,     \
                            unsigned>                                          \
    merge(K in_means[],                                                        \
          V in_weights[],                                                      \
          unsigned in_length,                                                  \
          K out_means[],                                                       \
          V out_weights[],                                                     \
          unsigned out_length)                                                 \
    {                                                                          \
        auto& functions = regurgitate::impl::functions::instance();            \
        return (functions.merge_##key_type##_##val_type##_impl(in_means,       \
                                                               in_weights,     \
                                                               in_length,      \
                                                               out_means,      \
                                                               out_weights,    \
                                                               out_length));   \
    }                                                                          \
    template <typename K, typename V>                                          \
    inline std::enable_if_t<std::conjunction_v<std::is_same<K, key_type>,      \
                                               std::is_same<V, val_type>>,     \
                            void>                                              \
    sort(                                                                      \
        K means[], V weights[], unsigned length, K scratch_m[], V scratch_w[]) \
    {                                                                          \
        auto& functions = regurgitate::impl::functions::instance();            \
        functions.sort_##key_type##_##val_type##_impl(                         \
            means, weights, length, scratch_m, scratch_w);                     \
    }

ISPC_MERGE_AND_SORT_WRAPPER(float, double)
ISPC_MERGE_AND_SORT_WRAPPER(float, float)

#undef ISPC_MERGE_AND_SORT_WRAPPER

} // namespace ispc

namespace regurgitate {

namespace detail {

template <typename Container> class centroid_container_iterator
{
    static constexpr size_t invalid_idx = std::numeric_limits<size_t>::max();
    std::reference_wrapper<Container> container_;
    size_t idx_ = invalid_idx;

public:
    using difference_type = std::ptrdiff_t;
    using value_type = typename Container::value_type;
    using pointer = void;
    using reference = value_type&;
    using iterator_category = std::forward_iterator_tag;

    centroid_container_iterator(Container& container, size_t idx = 0)
        : container_(container)
        , idx_(idx)
    {}

    centroid_container_iterator(const Container& container, size_t idx = 0)
        : container_(const_cast<Container>(container))
        , idx_(idx)
    {}

    centroid_container_iterator(const centroid_container_iterator& other)
        : container_(other.container_)
        , idx_(other.idx_)
    {}

    centroid_container_iterator&
    operator=(centroid_container_iterator const& other)
    {
        if (this != &other) {
            container_ = other.container_;
            idx_ = other.idx_;
        }
        return (*this);
    }

    bool operator==(centroid_container_iterator const& other)
    {
        return (idx_ == other.idx_);
    }

    bool operator!=(centroid_container_iterator const& other)
    {
        return (idx_ != other.idx_);
    }

    centroid_container_iterator& operator++()
    {
        idx_++;
        return (*this);
    }

    centroid_container_iterator& operator++(int)
    {
        auto to_return = *this;
        idx_++;
        return (to_return);
    }

    value_type operator*() { return (container_.get()[idx_]); }
};

template <typename ValueType, typename WeightType, size_t Size>
struct centroid_container
{
    using centroid = std::pair<ValueType, WeightType>;

    using iterator = centroid_container_iterator<
        centroid_container<ValueType, WeightType, Size>>;
    using const_iterator = const iterator;
    using difference_type = std::ptrdiff_t;
    using value_type = centroid;
    using reference_type = value_type&;
    using size_type = size_t;

    std::array<ValueType, Size> means_;
    std::array<WeightType, Size> weights_;
    size_t cursor_ = 0;

    inline void push_back(ValueType v, WeightType w)
    {
        return (push_back({v, w}));
    }

    inline void push_back(centroid&& x)
    {
        assert(x.second != 0);
        assert(cursor_ < Size);

        means_[cursor_] = x.first;
        weights_[cursor_] = x.second;

        cursor_++;
    }

    value_type operator[](size_t idx)
    {
        return (centroid{means_[idx], weights_[idx]});
    }

    value_type operator[](size_t idx) const
    {
        return (centroid{means_[idx], weights_[idx]});
    }

    void reset() { cursor_ = 0; }

    WeightType total_weight() const
    {
        return (std::accumulate(
            weights_.data(), weights_.data() + cursor_, WeightType{0}));
    }

    ValueType* means() { return (means_.data()); }

    WeightType* weights() { return (weights_.data()); }

    bool empty() const { return (cursor_ == 0); }

    size_t size() const { return (cursor_); }

    size_t capacity() const { return (Size); }

    centroid front() const { return ((*this)[0]); }

    centroid back() const { return ((*this)[size()]); }

    iterator begin() { return (iterator(*this)); }
    const_iterator begin() const { return (iterator(*this)); }

    iterator end() { return (iterator(*this, size())); }
    const_iterator end() const { return (iterator(*this, size())); }

    void dump()
    {
        std::cout << "length = " << size() << std::endl;
        std::cout << '[';
        std::for_each(begin(), end(), [](const auto& pair) {
            std::cout << "(" << pair.first << "," << pair.second << "), ";
        });
        std::cout << "\b\b]" << std::endl;
    }
};

} // namespace detail

template <typename ValueType, typename WeightType, size_t Size> class digest
{
    /*
     * Larger buffer sizes mean we calculate logarithmic bucket values less
     * often However, if the buffer is too large, then we'll spill out of the
     * CPU cache when merging.
     * Additionally, the 8-wide vector sorting implementation performs sorts
     * of 64 item chunks, so a multiples of 64 is the most efficient size.
     */
    static constexpr size_t buffer_size = 128;
    using buffer =
        detail::centroid_container<ValueType, WeightType, buffer_size>;
    buffer buffer_;

    using container = detail::centroid_container<ValueType, WeightType, Size>;
    container a_;
    container b_;
    container* active_ = &a_;

    ValueType min_ = std::numeric_limits<ValueType>::max();
    ValueType max_ = std::numeric_limits<ValueType>::lowest();

public:
    digest()
    {
        /* Initialize, i.e. benchmark, merge and sort functions */
        [[maybe_unused]] auto& functions =
            regurgitate::impl::functions::instance();
    }

    digest(const digest& other)
        : buffer_(other.buffer_)
        , a_(other.a_)
        , b_(other.b_)
        , active_(other.active_ == &other.a_ ? &a_ : &b_)
        , min_(other.min_)
        , max_(other.max_)
    {}

    digest& operator=(const digest& other)
    {
        if (this != &other) {
            buffer_ = other.buffer_;
            a_ = other.a_;
            b_ = other.b_;
            active_ = (other.active_ == other.a_ ? &a_ : &b_);
            min_ = other.min_;
            max_ = other.max_;
        }
        return (*this);
    }

    digest(digest&& other)
        : buffer_(std::move(other.buffer_))
        , a_(std::move(other.a_))
        , b_(std::move(other.b_))
        , active_(other.active_ == &other.a_ ? &a_ : &b_)
        , min_(other.min_)
        , max_(other.max_)
    {}

    digest& operator=(digest&& other)
    {
        if (this != &other) {
            buffer_ = std::move(other.buffer_);
            a_ = std::move(other.a_);
            b_ = std::move(other.b_);
            active_ = (other.active_ == other.a_ ? &a_ : &b_);
            min_ = other.min_;
            max_ = other.max_;
        }
        return (*this);
    }

    inline void insert(ValueType v) { insert(v, 1); }

    inline void insert(ValueType v, WeightType w)
    {
        buffer_.push_back(v, w);
        if (buffer_.size() >= buffer_size - Size) { merge(); }
    }

    void insert(const digest<ValueType, WeightType, Size>& src)
    {
        max_ = std::max(max_, src.max_);
        min_ = std::min(min_, src.min_);

        std::for_each(
            src.active_->begin(), src.active_->end(), [&](const auto& pair) {
                insert(pair.first, pair.second);
            });
        merge();
    }

    std::vector<typename container::centroid> get() const
    {
        std::vector<typename container::centroid> to_return;
        container* src = nullptr;

        do {
            to_return.clear();
            std::atomic_thread_fence(std::memory_order_consume);
            src = active_;
            std::copy(active_->begin(),
                      active_->end(),
                      std::back_inserter(to_return));
        } while (src != active_);

        return (to_return);
    }

    size_t centroid_count() const
    {
        assert(active_);
        return (active_->size());
    }

    size_t total_weight() const
    {
        assert(active_);
        return (active_->total_weight());
    }

    ValueType max() const { return (max_); }

    ValueType min() const { return (min_); }

    void merge()
    {
        if (buffer_.empty()) { return; }

        auto scratch = buffer{};
        auto* inactive = (&a_ == active_ ? &b_ : &a_);

        /* Add all active data to the end of the buffer */
        std::copy(
            active_->begin(), active_->end(), std::back_inserter(buffer_));

        /* Sort the buffer in place */
        ispc::sort(buffer_.means(),
                   buffer_.weights(),
                   buffer_.size(),
                   scratch.means(),
                   scratch.weights());

        /*
         * Min/max values are now at the ends of the buffer; compare against our
         * current values.
         */
        min_ = std::min(min_, buffer_.front().first);
        max_ = std::max(max_, buffer_.back().first);

        /* Merge buffer data and compress into the inactive container */
        inactive->cursor_ = ispc::merge(buffer_.means(),
                                        buffer_.weights(),
                                        buffer_.size(),
                                        inactive->means(),
                                        inactive->weights(),
                                        inactive->capacity());

        /* Swap the containers and clean up */
        std::swap(active_, inactive);
        std::atomic_thread_fence(std::memory_order_release);
        buffer_.reset();
        inactive->reset();
    }

    void dump() const { active_->dump(); }
};

} // namespace regurgitate

#endif /* _LIB_REGURGITATE_HPP_ */
