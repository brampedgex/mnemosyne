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

    template <memory_range Range = const_memory_span>
    class scanner {
    public:
        explicit scanner(Range range) noexcept : range_(std::move(range)) {}

        [[nodiscard]] memory_range_element_t<Range>* scan_signature(signature sig) const noexcept {
            // TODO: SIMD-based methods, which will be ez to make since we only support x86/64

            for (auto& i : range_) {
                auto result =
                        std::search(
                            std::execution::par_unseq,
                            i.begin(), i.end(),
                            sig.begin(), sig.end());

                if (result != i.end())
                    return std::to_address(result);
            }

            return nullptr;
        }

        [[nodiscard]] auto& range() noexcept { return range_; }
        [[nodiscard]] auto& range() const noexcept { return range_; }

    private:
        Range range_;
    };

    template <std::copyable Range>
    scanner(Range range) -> scanner<Range>;
}

#endif