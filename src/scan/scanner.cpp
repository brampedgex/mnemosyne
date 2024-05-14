#include "scanner_impls.hpp"
#include "../cpuid.hpp"

#include <cstring>
#include <execution>

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

        const std::byte* do_scan_normal_x16(const std::byte* begin, const std::byte* end, signature sig) {
            // Align begin to the next 16-byte boundary.
            begin = reinterpret_cast<const std::byte*>(
                (reinterpret_cast<uintptr_t>(begin) + 15) & ~static_cast<uintptr_t>(15));

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
    }

    const std::byte* scan_impl_normal(const std::byte* begin, const std::byte* end, signature sig, scan_align align) {
        while (sig.back().mask() == std::byte{0}) {
            sig = sig.subsig(0, sig.size() - 1);
            end--;
            // the sig cannot be empty. like that LITERALLY cannot happen. that would be stupid. dumb even.
        }

        if (align == scan_align::x16)
            return do_scan_normal_x16(begin, end, sig);

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

        auto iter = std::search(std::execution::unseq, begin, end, sig.begin(), sig.end());
        return iter == end ? nullptr : iter;
    }

    const std::byte* do_scan(const std::byte* begin, const std::byte* end, signature sig, scan_mode mode, scan_align align) {
        // TODO: Cover this in a test because it broke scanning until now
        size_t left_stripped = 0;
        if (align == scan_align::x1) {
            while (sig.front().mask() == std::byte{0}) {
                left_stripped++;
                sig = sig.subsig(1);
                begin++;
                if (sig.empty())
                    return (begin > end) ? nullptr : begin;
            }
        }

        if (begin >= end)
            return nullptr;

        const std::byte* result;
        switch (mode) {
            case scan_mode::normal:
            default:
                result = scan_impl_normal(begin, end, sig, align);
            case scan_mode::avx2:
                result = scan_impl_avx2(begin, end, sig, align);
        }

        return result ? result - left_stripped : nullptr;
    }
}
