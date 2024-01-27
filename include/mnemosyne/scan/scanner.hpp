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
    // TODO: Result type instead of pointer

    namespace internal {
        const std::byte* do_scan(const std::byte* begin, const std::byte* end, const signature& sig);

        inline std::byte* do_scan(std::byte* begin, std::byte* end, const signature& sig) {
            return const_cast<std::byte*>(do_scan( // rare const_cast use case?!?!?!
                    static_cast<const std::byte*>(begin),
                    static_cast<const std::byte*>(end),
                    sig));
        }
    }

    template <memory_range Range = const_memory_span>
    class scanner {
    public:
        explicit scanner(Range range) noexcept : range_(std::move(range)) {}

        [[nodiscard]] memory_range_element_t<Range>* scan_signature(signature sig) const noexcept {
            // TODO: SIMD-based methods, which will be ez to make since we only support x86/64

            for (auto& i : range_) {
                if (auto result = internal::do_scan(std::to_address(i.begin()), std::to_address(i.end()), sig); result)
                    return result;
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