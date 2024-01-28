#pragma once
#include <mnemosyne/scan/scanner.hpp>

namespace mnem::internal {
    const std::byte* scan_impl_normal(const std::byte* begin, const std::byte* end, signature sig);
    const std::byte* scan_impl_avx2(const std::byte* begin, const std::byte* end, signature sig);
}