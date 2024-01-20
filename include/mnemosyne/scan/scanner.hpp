#pragma once
#ifndef MNEMOSYNE_SCAN_SCANNER_HPP
#define MNEMOSYNE_SCAN_SCANNER_HPP

#include "signature.hpp"
#include "../core/memory_range.hpp"

#include <algorithm>
#include <execution>

namespace mnem {
    // TODO: Execution policies to give the user better control over threading
    // TODO: Scan alignment for results we know are aligned in memory
    // TODO: Variants supporting multiple results and batches of signatures
    // TODO: Scan for scalar types and simple byte arrays
    // TODO: Result type instead of uintptr_t

    template <std::copyable Range = const_memory_range>
    class scanner {
    public:
        explicit scanner(Range range) noexcept : range_(std::move(range)) {}

        [[nodiscard]] auto* scan_signature(signature sig) const noexcept {
            // TODO: SIMD-based methods, which will be ez to make since we only support x86/64
            auto res = std::search(
                    std::execution::par_unseq,
                    range_.begin(), range_.end(),
                    sig.begin(), sig.end());

            if (res == range_.end())
                return decltype(res){nullptr};
            else
                return res;
        }

        [[nodiscard]] memory_range& range() noexcept { return range_; }
        [[nodiscard]] const memory_range& range() const noexcept { return range_; }

    private:
        Range range_;
    };

    template <std::copyable Range>
    scanner(Range range) -> scanner<Range>;
}

#endif