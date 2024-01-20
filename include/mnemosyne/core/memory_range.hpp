#ifndef MNEMOSYNE_CORE_MEMORY_RANGE_HPP
#define MNEMOSYNE_CORE_MEMORY_RANGE_HPP

#include <cstddef>

namespace mnem {
    /// Represents a range in memory.
    class memory_range {
    public:
        constexpr memory_range(std::byte* begin, std::byte* end) noexcept : begin_(begin), end_(end) {}

        [[nodiscard]] auto* begin() const noexcept { return begin_; }
        [[nodiscard]] auto* end() const noexcept { return end_; }

    private:
        std::byte *begin_, *end_;
    };
}

#endif