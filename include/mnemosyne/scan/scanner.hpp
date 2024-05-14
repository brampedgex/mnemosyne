#pragma once
#ifndef MNEMOSYNE_SCAN_SCANNER_HPP
#define MNEMOSYNE_SCAN_SCANNER_HPP

#include "signature.hpp"
#include "../core/memory_range.hpp"

#include <algorithm>

namespace mnem {
    // TODO: Execution policies to give the user better control over threading
    // TODO: Scan alignment for results we know are aligned in memory
    // TODO: Variants supporting multiple results and batches of signatures
    // TODO: Scan for scalar types and simple byte arrays
    // TODO: Result type instead of pointer

    /// Describes which scanning mode to use.
    enum class scan_mode : int {
        // Pure C++ scanner, which uses no CPU extensions or intrinsics.
        normal,
        // 128-bit SSE 4.2 scanner, works on almost all modern CPUs (not implemented)
        sse4_2,
        // 256-bit AVX2 scanner, works on most modern CPUs
        avx2,
        // 512-bit AVX512 scanner, only works on the most recent CPUs (not implemented)
        avx512,
        // Maximum possible value of this enum
        max,
    };

    /// Describes the desired alignment for a signature result.
    enum class scan_align : int {
        // Not aligned
        x1,
        // 16-byte aligned, which is useful when you are finding the start of x86 functions.
        x16,
    };

    scan_mode detect_scan_mode();

    namespace internal {
        const std::byte* do_scan(const std::byte* begin, const std::byte* end, signature sig, scan_mode mode, scan_align align);

        inline std::byte* do_scan(std::byte* begin, std::byte* end, signature sig, scan_mode mode, scan_align align) {
            return const_cast<std::byte*>(do_scan( // rare const_cast use case?!?!?!
                    static_cast<const std::byte*>(begin),
                    static_cast<const std::byte*>(end),
                    sig,
                    mode,
                    align));
        }
    }

    template <memory_range Range = const_memory_span>
    class scanner {
    public:
        explicit scanner(Range range, scan_mode mode = detect_scan_mode()) noexcept : range_(std::move(range)), mode_(mode) {}

        [[nodiscard]] memory_range_element_t<Range>* scan_signature(signature sig, scan_align align = scan_align::x1) const noexcept {
            if (sig.container().empty())
                return nullptr;

            for (auto& i : range_) {
                if (std::distance(i.begin(), i.end()) < sig.size())
                    continue;

                if (auto result = internal::do_scan(std::to_address(i.begin()), std::to_address(i.end()), sig, mode_, align); result)
                    return result;
            }

            return nullptr;
        }

        [[nodiscard]] auto mode() const noexcept { return mode_; }
        void set_mode(scan_mode mode = detect_scan_mode()) {
            mode_ = mode;
        }

        [[nodiscard]] auto& range() noexcept { return range_; }
        [[nodiscard]] auto& range() const noexcept { return range_; }

    private:
        scan_mode mode_;
        Range range_;
    };

    template <std::copyable Range>
    scanner(Range range) -> scanner<Range>;
}

#endif
