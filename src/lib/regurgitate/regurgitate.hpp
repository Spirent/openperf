#ifndef _LIB_REGURGITATE_HPP_
#define _LIB_REGURGITATE_HPP_

#include <algorithm>
#include <array>
#include <atomic>
#include <cassert>
#include <numeric>

#include <iterator>
#include <iostream>

#include "api.h"

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
        return (regurgitate_merge_##key_type##_##val_type(in_means,            \
                                                          in_weights,          \
                                                          in_length,           \
                                                          out_means,           \
                                                          out_weights,         \
                                                          out_length));        \
    }                                                                          \
    template <typename K, typename V>                                          \
    inline std::enable_if_t<std::conjunction_v<std::is_same<K, key_type>,      \
                                               std::is_same<V, val_type>>,     \
                            void>                                              \
    sort(                                                                      \
        K means[], V weights[], unsigned length, K scratch_m[], V scratch_w[]) \
    {                                                                          \
        regurgitate_sort_##key_type##_##val_type(                              \
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
        : container_(const_cast<Container&>(container))
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

/*
 * This digest class is designed to be used by a single reader and a single
 * writer. Only writers should call the insert function. Readers may call
 * anything else.
 */
template <typename ValueType, typename WeightType, size_t Size> class digest
{
    using buffer = detail::
        centroid_container<ValueType, WeightType, regurgitate_optimum_length>;
    buffer buffer_;

    using container = detail::centroid_container<ValueType, WeightType, Size>;
    container a_;
    container b_;
    container* active_ = &a_;

    /* Merge the current digest with the incoming data in the buffer. */
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

public:
    digest()
    {
        /* Perform run-time benchmarking if we haven't already */
        regurgitate_init();
    }

    digest(const digest& other)
        : buffer_(other.buffer_)
        , a_(other.a_)
        , b_(other.b_)
        , active_(other.active_ == &other.a_ ? &a_ : &b_)
    {}

    digest& operator=(const digest& other)
    {
        if (this != &other) {
            buffer_ = other.buffer_;
            a_ = other.a_;
            b_ = other.b_;
            active_ = (other.active_ == other.a_ ? &a_ : &b_);
        }
        return (*this);
    }

    digest(digest&& other) noexcept
        : buffer_(std::move(other.buffer_))
        , a_(std::move(other.a_))
        , b_(std::move(other.b_))
        , active_(other.active_ == &other.a_ ? &a_ : &b_)
    {}

    digest& operator=(digest&& other) noexcept
    {
        if (this != &other) {
            buffer_ = std::move(other.buffer_);
            a_ = std::move(other.a_);
            b_ = std::move(other.b_);
            active_ = (other.active_ == other.a_ ? &a_ : &b_);
        }
        return (*this);
    }

    inline void insert(ValueType v) { insert(v, 1); }

    inline void insert(ValueType v, WeightType w)
    {
        buffer_.push_back(v, w);

        /*
         * We need to be able to add up to Size items to the buffer
         * in order to merge.
         */
        if (buffer_.size() >= regurgitate_optimum_length - Size) { merge(); }
    }

    /* Retrieve an accurate view of the current data set. */
    void get_snapshot(container& snapshot) const
    {
        assert(snapshot.empty());

        auto data = buffer{};
        auto scratch = buffer{};
        container* src = nullptr;
        bool need_merge;

        /*
         * We only need to perform a merge if the buffer contains
         * unprocessed data points.
         */
        do {
            need_merge = false;
            std::atomic_thread_fence(std::memory_order_consume);
            src = active_;
            if (!buffer_.empty()) {
                std::copy(std::begin(buffer_),
                          std::end(buffer_),
                          std::back_inserter(data));
                need_merge = true;
            }
            std::copy(std::begin(*active_),
                      std::end(*active_),
                      std::back_inserter(data));
        } while (src != active_);

        if (need_merge) {
            ispc::sort(data.means(),
                       data.weights(),
                       data.size(),
                       scratch.means(),
                       scratch.weights());

            snapshot.cursor_ = ispc::merge(data.means(),
                                           data.weights(),
                                           data.size(),
                                           snapshot.means(),
                                           snapshot.weights(),
                                           snapshot.capacity());

        } else {
            std::copy(
                std::begin(data), std::end(data), std::back_inserter(snapshot));
        }
    }

    std::vector<typename container::centroid> get() const
    {
        /* Get a snapshot... */
        auto tmp = container{};
        get_snapshot(tmp);

        /* ... and copy it out */
        auto to_return = std::vector<typename container::centroid>{};
        to_return.reserve(tmp.size());
        std::copy(
            std::begin(tmp), std::end(tmp), std::back_inserter(to_return));
        return (to_return);
    }

    void dump() const { active_->dump(); }
};

} // namespace regurgitate

#endif /* _LIB_REGURGITATE_HPP_ */
