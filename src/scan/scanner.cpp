#include "scanner_impls.hpp"
#include "../cpuid.hpp"

#include <cstring>

namespace mnem {
    scan_mode detect_scan_mode() {
        using internal::cpuinfo;

        if (cpuinfo::bmi1() && cpuinfo::avx2())
            return scan_mode::avx2;

        if (cpuinfo::sse4_2())
            return scan_mode::sse4_2;

        return scan_mode::normal;
    }
}

namespace mnem::internal {
    namespace {
        auto find_byte(const std::byte* first, const std::byte* last, std::byte byte) {
            // libstdc++'s std::find doesn't use memchr, unlike microsoft's implementation. Use memchr directly instead.
            return reinterpret_cast<const std::byte*>(
                std::memchr(first, static_cast<int>(byte), last - first));
        }
    }

    const std::byte* scan_impl_normal_x1(const std::byte* begin, const std::byte* end, signature sig) {
        const auto first_elem = sig.front();
        if (first_elem.mask() == std::byte{0xFF}) {
            const auto first = first_elem.byte();

            const auto upper_bound = end - (sig.size() - 1);
            auto ptr = find_byte(begin, upper_bound, first);

            while (ptr) [[likely]] {
                if (std::equal(sig.begin(), sig.end(), ptr)) [[unlikely]] {
                    return ptr;
                }

                ptr = find_byte(ptr + 1, upper_bound, first);
            }

            return nullptr;
        }

        auto iter = std::search(begin, end, sig.begin(), sig.end());
        return iter == end ? nullptr : iter;
    }

    const std::byte* scan_impl_normal_x16(const std::byte* begin, const std::byte* end, signature sig) {
        // Make `end` the upper bound for where the first byte can be located.
        end -= sig.size() - 1;

        const auto first_elem = sig.front();
        if (first_elem.mask() == std::byte{0xFF}) {
            const auto first = first_elem.byte();

            auto ptr = find_byte(begin, end, first);

            while (ptr) [[likely]] {
                if (reinterpret_cast<uintptr_t>(ptr) % 16 == 0) [[unlikely]] {
                    if (std::equal(sig.begin(), sig.end(), ptr)) [[unlikely]] {
                        return ptr;
                    }
                }

                ptr = find_byte(ptr + 1, end, first);
            }

            return nullptr;
        }

        for (auto ptr = begin; ptr < end; ptr += 16) {
            if (std::equal(sig.begin(), sig.end(), ptr)) [[unlikely]] {
                return ptr;
            }
        }

        return nullptr;
    }

    const std::byte* do_scan(const std::byte* begin, const std::byte* end, signature sig, scan_mode mode, scan_align align) {
        // TODO: Cover this logic in a test because it broke scanning until now
        size_t left_stripped = 0;
        if (align == scan_align::x1) {
            while (sig.front().mask() == std::byte{0}) {
                left_stripped++;
                sig = sig.subsig(1);
                begin++;
                if (sig.empty())
                    return (begin > end) ? nullptr : begin - left_stripped;
            }
        } else {
            begin = align_ptr_up<16>(begin);
        }

        while (sig.back().mask() == std::byte{0}) {
            sig = sig.subsig(0, sig.size() - 1);
            end--;
            // We know there must be at least one non-masked byte in the signature, so no need to check if sig is empty
        }

        if (begin >= end)
            return nullptr;

        const std::byte* result;

        if (align == scan_align::x1) {
            switch (mode) {
                case scan_mode::normal:
                default:
                    result = scan_impl_normal_x1(begin, end, sig);
                    break;
                case scan_mode::avx2:
                    result = scan_impl_avx2_x1(begin, end, sig);
                    break;
            }
        } else {
            switch (mode) {
                case scan_mode::normal:
                default:
                    result = scan_impl_normal_x16(begin, end, sig);
                    break;
                case scan_mode::avx2:
                    result = scan_impl_avx2_x16(begin, end, sig);
                    break;
            }
        }

        return result ? result - left_stripped : nullptr;
    }
}
