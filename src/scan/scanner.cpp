#include <mnemosyne/scan/scanner.hpp>

namespace mnem::internal {
    const std::byte* do_scan(const std::byte* begin, const std::byte* end, const signature& sig, scan_mode mode) {
        return std::search(begin, end, sig.begin(), sig.end());
    }
}