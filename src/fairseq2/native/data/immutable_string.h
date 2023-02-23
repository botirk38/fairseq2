// Copyright (c) Meta Platforms, Inc. and affiliates.
// All rights reserved.
//
// This source code is licensed under the BSD-style license found in the
// LICENSE file in the root directory of this source tree.

#pragma once

#include <cstddef>
#include <string>
#include <string_view>
#include <utility>

#include "fairseq2/native/api.h"
#include "fairseq2/native/memory.h"

namespace fairseq2 {

// An immutable, ref-counted UTF-8 string type.
class FAIRSEQ2_API immutable_string {
public:
    using value_type             = std::string_view::value_type;
    using size_type              = std::string_view::size_type;
    using difference_type        = std::string_view::difference_type;
    using reference              = std::string_view::reference;
    using const_reference        = std::string_view::const_reference;
    using pointer                = std::string_view::pointer;
    using const_pointer          = std::string_view::const_pointer;
    using iterator               = std::string_view::iterator;
    using const_iterator         = std::string_view::const_iterator;
    using reverse_iterator       = std::string_view::reverse_iterator;
    using const_reverse_iterator = std::string_view::const_reverse_iterator;

public:
    immutable_string() noexcept = default;

    immutable_string(const char *s)
        : immutable_string{std::string_view{s}}
    {}

    immutable_string(const std::string &s)
        : immutable_string{static_cast<std::string_view>(s)}
    {}

    immutable_string(std::string_view s);

    explicit
    immutable_string(memory_block storage) noexcept
        : storage_{std::move(storage)}
    {}

    immutable_string(const immutable_string &other) noexcept
    {
        storage_ = other.storage_.share();
    }

    immutable_string &
    operator=(const immutable_string &other) noexcept
    {
        if (this != &other)
            storage_ = other.storage_.share();

        return *this;
    }

    immutable_string(immutable_string &&) noexcept = default;
    immutable_string &operator=(immutable_string &&) noexcept = default;

   ~immutable_string() = default;

    const_iterator
    begin() const noexcept
    {
        return view().begin();
    }

    const_iterator
    end() const noexcept
    {
        return view().end();
    }

    const_reverse_iterator
    rbegin() const noexcept
    {
        return view().rbegin();
    }

    const_reverse_iterator
    rend() const noexcept
    {
        return view().rend();
    }

    const_reference
    operator[](size_type pos) const
    {
        std::string_view v = view();

        return v[pos];
    }

    const_pointer
    data() const noexcept
    {
        return view().data();
    }

    size_type
    size() const noexcept
    {
        return view().size();
    }

    bool
    empty() const noexcept
    {
        return view().empty();
    }

    friend bool
    operator==(const immutable_string &lhs, const immutable_string &rhs) noexcept
    {
        return lhs.view() == rhs.view();
    }

    friend bool
    operator!=(const immutable_string &lhs, const immutable_string &rhs) noexcept
    {
        return lhs.view() != rhs.view();
    }

    immutable_string
    remove_prefix(size_type n) const noexcept
    {
        if (n == 0)
            return *this;

        return immutable_string{storage_.share_slice(n)};
    }

    immutable_string
    remove_suffix(size_type n) const noexcept
    {
        if (n == 0)
            return *this;

        return immutable_string{storage_.share_slice(0, storage_.size() - n)};
    }

    operator std::string_view() const noexcept
    {
        return view();
    }

    std::string
    to_string() const
    {
        return std::string{*this};
    }

    std::size_t
    get_code_point_length() const;

private:
    static memory_block
    copy_string(std::string_view s);

    std::string_view
    view() const noexcept
    {
        auto chars = storage_.cast<const value_type>();

        return {chars.data(), chars.size()};
    }

private:
    memory_block storage_{};
};

}  // namespace fairseq2

template <>
struct std::hash<fairseq2::immutable_string> {
    inline std::size_t
    operator()(const fairseq2::immutable_string &v) const noexcept
    {
        return std::hash<std::string_view>{}(v);
    }
};