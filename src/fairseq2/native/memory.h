// Copyright (c) Meta Platforms, Inc. and affiliates.
// All rights reserved.
//
// This source code is licensed under the BSD-style license found in the
// LICENSE file in the root directory of this source tree.

#pragma once

#include <cstddef>
#include <memory>
#include <string>
#include <type_traits>

#include "fairseq2/native/api.h"
#include "fairseq2/native/span.h"

namespace fairseq2 {

// Besides `addr` and `size`, the signature also allows passing an opaque `ctx`
// pointer. It is typically used to pass extra information required to release
// the underlying memory.
using memory_deallocator = void (*)(const void *addr, std::size_t size, void *ctx) noexcept;

// Used internally by `memory_block` to manage the lifetime of the memory.
class FAIRSEQ2_API memory_holder {
public:
    explicit
    memory_holder(const void *addr, std::size_t size, void *ctx, memory_deallocator d) noexcept
        : addr_{addr}, size_{size}, ctx_{ctx}, deallocate_{d}
    {}

    memory_holder(const memory_holder &) = delete;
    memory_holder &operator=(const memory_holder &) = delete;

    memory_holder(memory_holder &&) = delete;
    memory_holder &operator=(memory_holder &&) = delete;

   ~memory_holder()
    {
        if (deallocate_ != nullptr)
            deallocate_(addr_, size_, ctx_);
    }

private:
    const void *addr_;
    std::size_t size_;
    void *ctx_;
    memory_deallocator deallocate_;
};

// A `memory_block` is intended to be used for zero-copy memory sharing. The
// underlying memory is ref-counted and is freed when no longer needed.
template <typename T>
class basic_memory_block {

    template <typename U>
    friend class basic_memory_block;

public:
    using element_type    = T;
    using value_type      = std::remove_cv_t<T>;
    using size_type       = std::size_t;
    using difference_type = std::ptrdiff_t;
    using reference       = T &;
    using const_reference = const T &;
    using pointer         = T *;
    using const_pointer   = const T *;
    using iterator        = T *;
    using const_iterator  = const T *;

public:
    basic_memory_block() noexcept = default;

    explicit
    basic_memory_block(pointer data, size_type size) noexcept
        : data_{data}, size_{size}
    {}

    explicit
    basic_memory_block(pointer data, size_type size, void *ctx, memory_deallocator d);

    template <typename U>
    basic_memory_block(const basic_memory_block<U> &other) noexcept
        : data_{other.data_}, size_{other.size_}, holder_{other.holder_}
    {
        static_assert(std::is_convertible_v<U *, T *>,
            "A `memory_block` cannot be converted into a `writable_memory_block`.");
    }

    basic_memory_block(const basic_memory_block &) noexcept = default;
    basic_memory_block &operator=(const basic_memory_block &) noexcept = default;

    basic_memory_block(basic_memory_block &&) noexcept = default;
    basic_memory_block &operator=(basic_memory_block &&) noexcept = default;

   ~basic_memory_block() = default;

    basic_memory_block
    share_slice(size_type offset) const noexcept
    {
        return basic_memory_block{*this, data_ + offset, size_ - offset};
    }

    basic_memory_block
    share_slice(size_type offset, size_type count) const noexcept
    {
        return basic_memory_block{*this, data_ + offset, count};
    }

    basic_memory_block
    share_first(size_type count) const noexcept
    {
        return basic_memory_block{*this, data_, count};
    }

    basic_memory_block
    share_last(size_type count) const noexcept
    {
        return basic_memory_block{*this, data_ + size_ - count, count};
    }

    iterator
    begin() const noexcept
    {
        return data_;
    }

    iterator
    end() const noexcept
    {
        return data_ + size_;
    }

    pointer
    data() const noexcept
    {
        return data_;
    }

    size_type
    size() const noexcept
    {
        return size_;
    }

    bool
    empty() const noexcept
    {
        return size_ == 0;
    }

    template <typename U>
    span<U>
    cast() const noexcept
    {
        static_assert(std::is_const_v<U> || !std::is_const_v<T>,
            "A `memory_block` cannot be cast to a non-const type.");

        return {reinterpret_cast<U *>(data_), size_ / sizeof(U)};
    }

private:
    explicit
    basic_memory_block(const basic_memory_block &other, pointer data, std::size_t size) noexcept
        : data_{data}, size_{size}
    {
        if (size_ > 0)
            holder_ = other.holder_;
    }

private:
    pointer data_ = nullptr;
    size_type size_ = 0;
    std::shared_ptr<memory_holder> holder_{};
};

template <typename T>
basic_memory_block<T>::basic_memory_block(
    pointer data, size_type size, void *ctx, memory_deallocator d
)
    : data_{data}, size_{size}
{
    if (size_ > 0) {
        // As a contract, we take the ownership of `data`. This means we have to
        // make sure that we don't leak in case of a failure.
        try {
            holder_ = std::make_shared<memory_holder>(data, size, ctx, d);
        } catch (...) {
            if (d != nullptr)
                d(data, size, ctx);

            throw;
        }
    }
}

using memory_block = basic_memory_block<const std::byte>;
using writable_memory_block = basic_memory_block<std::byte>;

using memory_span = span<const std::byte>;
using writable_memory_span = span<std::byte>;

template <typename T>
inline span<T>
cast(memory_span s) noexcept
{
    static_assert(std::is_const_v<T>,
        "`T` must be const.");

    return {reinterpret_cast<T *>(s.data()), s.size() / sizeof(T)};
}

template <typename T>
inline span<T>
cast(writable_memory_span s) noexcept
{
    return {reinterpret_cast<T *>(s.data()), s.size() / sizeof(T)};
}

FAIRSEQ2_API writable_memory_block
allocate_memory(std::size_t size);

FAIRSEQ2_API writable_memory_block
copy_memory(memory_span src);

}  // namespace fairseq2
