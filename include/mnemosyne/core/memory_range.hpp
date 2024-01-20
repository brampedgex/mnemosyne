#pragma once
#ifndef MNEMOSYNE_CORE_MEMORY_RANGE_HPP
#define MNEMOSYNE_CORE_MEMORY_RANGE_HPP

#include <cstddef>

namespace mnem {
    /// Represents a range in memory.
    class memory_range {
    public:
        constexpr memory_range(std::byte* begin, std::byte* end) noexcept : begin_(begin), end_(end) {}
        constexpr memory_range(std::byte* begin, std::size_t size) noexcept : begin_(begin), end_(begin + size) {}

        [[nodiscard]] auto* begin() const noexcept { return begin_; }
        [[nodiscard]] auto* end() const noexcept { return end_; }

    private:
        std::byte *begin_, *end_;
    };

    class const_memory_range {
    public:
        constexpr const_memory_range(const std::byte* begin, const std::byte* end) noexcept : begin_(begin), end_(end) {}
        constexpr const_memory_range(const std::byte* begin, std::size_t size) noexcept : begin_(begin), end_(begin + size) {}
        constexpr const_memory_range(const memory_range& range) noexcept : begin_(range.begin()), end_(range.end()) {} // NOLINT(google-explicit-constructor)

        [[nodiscard]] auto* begin() const noexcept { return begin_; }
        [[nodiscard]] auto* end() const noexcept { return end_; }

    private:
        const std::byte *begin_, *end_;
    };
}

#endif