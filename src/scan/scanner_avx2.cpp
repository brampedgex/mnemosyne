#include "scanner_impls.hpp"

#include <iostream>
#include <bit>

#include <immintrin.h>

namespace mnem::internal {
    namespace {
        enum class cmp_type {
            none,   // Fully masked or not present
            full,   // Fully unmasked
            masked  // Partially masked
        };

        //enum class cmp_type {
        //    none,       // Don't compare
        //    vector,     // Do vectorized compare
        //    extended,   // Do vectorized compare, then std::equal
        //};

        // Find the optimal index in the two-byte search.
        size_t find_twobyte_idx(mnem::signature sig) {
            // This is good for random, uniformly distributed data, but in x86 code certain sequences of bytes are significantly more common that others.
            // In the future we might want a lookup table of more common byte sequences to avoid.

            size_t best = 0; // default to first byte

            if (sig[best].mask() == std::byte{0xFF} && sig.size() > 1 && sig[best + 1].mask() == std::byte{0xFF})
                return best;

            for (size_t i = 1; i < sig.size(); i++) {
                if (sig[i].mask() == std::byte{0})
                    continue;

                // Pick the position with a stricter mask.
                int i_count = std::popcount(static_cast<uint8_t>(sig[i].mask()));
                if (i + 1 < sig.size())
                    i_count += std::popcount(static_cast<uint8_t>(sig[i+1].mask()));

                int best_count = std::popcount(static_cast<uint8_t>(sig[best].mask()));
                best_count += std::popcount(static_cast<uint8_t>(sig[best+1].mask()));

                if (i_count > best_count)
                    best = i;
            }

            return best;
        }

        // Load signature bytes and masks into two 256-bit registers
        std::pair<std::array<std::byte, 32>, std::array<std::byte, 32>> load_sig_256(std::span<const mnem::sig_element> sig) {
            std::array<std::byte, 32> bytes{};
            std::array<std::byte, 32> masks{};

            for (size_t i = 0; i < 32; i++) {
                if (i < sig.size()) {
                    auto byte = sig[i].byte();
                    auto mask = sig[i].mask();
                    bytes[i] = byte & mask;
                    masks[i] = mask;
                } else {
                    bytes[i] = std::byte{0};
                    masks[i] = std::byte{0};
                }
            }

            return { bytes, masks };
        }

        template <cmp_type C0, cmp_type C1, cmp_type CSig, bool SigExt>
        const std::byte* do_scan_avx2_x1(const std::byte* begin, const std::byte* end, mnem::signature sig, size_t twobyte_idx) {
            __m256i b0, m0, b1, m1, bsig, msig;

            {
                b0 = _mm256_set1_epi8(static_cast<char>(sig[twobyte_idx].byte()));
                if constexpr (C0 == cmp_type::masked)
                    m0 = _mm256_set1_epi8(static_cast<char>(sig[twobyte_idx].mask()));
            }

            if constexpr (C1 != cmp_type::none) {
                b1 = _mm256_set1_epi8(static_cast<char>(sig[twobyte_idx+1].byte()));
                if constexpr (C1 == cmp_type::masked)
                    m1 = _mm256_set1_epi8(static_cast<char>(sig[twobyte_idx+1].mask()));
            }

            if constexpr (CSig != cmp_type::none) {
                auto [bytes, masks] = load_sig_256(sig);
                bsig = _mm256_loadu_si256(reinterpret_cast<__m256i*>(std::to_address(bytes.begin())));
                msig = _mm256_loadu_si256(reinterpret_cast<__m256i*>(std::to_address(masks.begin())));
            }

            static constexpr uintptr_t PAGE_MASK = ~static_cast<uintptr_t>(4095);
            // The "danger zone". If we read past here, a segfault can occur.
            auto end_page = (reinterpret_cast<uintptr_t>(end) + 4095) | PAGE_MASK;

            end -= sig.size() - twobyte_idx - 1;
            auto ptr = begin + twobyte_idx;

            // First do a partial compare up to the next 32-byte-aligned position. If `ptr` is already aligned, no harm done.
            {
                // This can't read past `end_page`, because we would fall back to normal scan if the search space was that small.
                auto mem = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(ptr));

                uint32_t mask;
                if constexpr (C0 == cmp_type::masked)
                    mask = _mm256_movemask_epi8(_mm256_cmpeq_epi8(_mm256_and_si256(mem, m0), b0));
                else
                    mask = _mm256_movemask_epi8(_mm256_cmpeq_epi8(mem, b0));

                mask &= _bzhi_u32(0xFFFFFFFF, 32 - (reinterpret_cast<uintptr_t>(ptr) | 31));

                if constexpr (C1 != cmp_type::none) {
                    uint32_t mask1;
                    if constexpr (C1 == cmp_type::masked)
                        mask1 = _mm256_movemask_epi8(_mm256_cmpeq_epi8(_mm256_and_si256(mem, m1), b1));
                    else
                        mask1 = _mm256_movemask_epi8(_mm256_cmpeq_epi8(mem, b1));

                    mask &= (mask1 >> 1) | (1u << 31); // avoid a second memory read
                }

                // `mask` is a bitmask telling us which positions matched the first two bytes.
                // Iterate over each set bit using BMI intrinsics, and do further checks for a match.
                while (mask) {
                    auto match = ptr + _tzcnt_u32(mask);

                    // match cannot reach past the end

                    if constexpr (CSig == cmp_type::none)
                        return match; // twobyte_idx is always 0 when CSig is `none`

                    match -= twobyte_idx;

                    auto match_vec = _mm256_and_si256(_mm256_loadu_si256(reinterpret_cast<const __m256i*>(match)), msig);
                    if (_mm256_movemask_epi8(_mm256_cmpeq_epi8(match_vec, bsig)) == 0xFFFFFFFF) {
                        if constexpr (!SigExt)
                            return match;

                        if (std::equal(sig.begin() + 32, sig.end(), match + 32))
                            return match;
                    }

                    mask = _blsr_u32(mask);
                }
            }

            ptr = reinterpret_cast<const std::byte*>((reinterpret_cast<uintptr_t>(ptr) | 31) + 1);

            for (; ptr < end; ptr += 32) {
                auto mem = _mm256_load_si256(reinterpret_cast<const __m256i*>(ptr));

                uint32_t mask;
                if constexpr (C0 == cmp_type::masked)
                    mask = _mm256_movemask_epi8(_mm256_cmpeq_epi8(_mm256_and_si256(mem, m0), b0));
                else
                    mask = _mm256_movemask_epi8(_mm256_cmpeq_epi8(mem, b0));

                if constexpr (C1 != cmp_type::none) {
                    uint32_t mask1;
                    if constexpr (C1 == cmp_type::masked)
                        mask1 = _mm256_movemask_epi8(_mm256_cmpeq_epi8(_mm256_and_si256(mem, m1), b1));
                    else
                        mask1 = _mm256_movemask_epi8(_mm256_cmpeq_epi8(mem, b1));

                    mask &= (mask1 >> 1) | (1u << 31); // avoid a second memory read
                }

                // `mask` is a bitmask telling us which positions matched the first two bytes.
                // Iterate over each set bit using BMI intrinsics, and do further checks for a match.
                while (mask) {
                    auto match = ptr + _tzcnt_u32(mask);

                    if (match > end)
                        return nullptr;

                    if constexpr (CSig == cmp_type::none)
                        return match; // twobyte_idx is always 0 when CSig is `none`

                    match -= twobyte_idx;

                    if constexpr (!SigExt) {
                        // If the signature is less than 32 bytes, we could still read past `end_page`.
                        // TODO: Probably better to adjust `end` more and just do this logic at the end of the function, to avoid an extra branch
                        //if (((reinterpret_cast<uintptr_t>(match) + 31) | PAGE_MASK) < end_page) {
                        if (true) {
                            auto match_vec = _mm256_and_si256(_mm256_loadu_si256(reinterpret_cast<const __m256i*>(match)), msig);
                            if (_mm256_movemask_epi8(_mm256_cmpeq_epi8(match_vec, bsig)) == 0xFFFFFFFF)
                                return match;
                        } else {
                            if (std::equal(sig.begin(), sig.end(), match))
                                return match;
                        }
                    } else {
                        auto match_vec = _mm256_and_si256(_mm256_loadu_si256(reinterpret_cast<const __m256i*>(match)), msig);
                        if (_mm256_movemask_epi8(_mm256_cmpeq_epi8(match_vec, bsig)) == 0xFFFFFFFF) {
                            if (std::equal(sig.begin() + 32, sig.end(), match + 32))
                                return match;
                        }
                    }

                    mask = _blsr_u32(mask);
                }
            }

            return nullptr;
        }
    }

    const std::byte* scan_impl_avx2(const std::byte* begin, const std::byte* end, signature sig) {
        auto twobyte_idx = find_twobyte_idx(sig);
        //auto twobyte_idx = 0;

        if (end - begin - twobyte_idx < 64) // pretty much not worth it if the buffer is that small
            return scan_impl_normal(begin, end, sig);

        // Strip bytes until they will fit into the AVX registers.
        while (sig.back().mask() == std::byte{0}) {
            sig = sig.subsig(0, sig.size() - 1);
            end--;
            // can't become empty
        }

        bool c0_masked = false;
        cmp_type c1 = cmp_type::none;
        bool csig = false;
        bool sigext = false;

        if (sig[twobyte_idx].mask() != std::byte{0xFF})
            c0_masked = true;

        if (twobyte_idx + 1 < sig.size()) {
            auto mask1 = sig[twobyte_idx + 1].mask();
            if (mask1 == std::byte{0xFF})
                c1 = cmp_type::full;
            else if (mask1 != std::byte{0})
                c1 = cmp_type::masked;
        }

        if (sig.size() > 2)
            csig = true;

        if (sig.size() > 32)
            sigext = true;

        const std::byte* result = nullptr;

        // Dispatch to the correct scanner func using epic lambda chain
        if (begin < end) {
            auto dispatch_2 = [&]<cmp_type C0, cmp_type C1> {
                if (csig) {
                    if (sigext)
                        result = do_scan_avx2_x1<C0, C1, cmp_type::full, true>(begin, end, sig, twobyte_idx);
                    else
                        result = do_scan_avx2_x1<C0, C1, cmp_type::full, false>(begin, end, sig, twobyte_idx);
                } else {
                    result = do_scan_avx2_x1<C0, C1, cmp_type::none, false>(begin, end, sig, twobyte_idx);
                }
            };

            auto dispatch_1 = [&]<cmp_type C0> {
                switch (c1) {
                    case cmp_type::none:
                        dispatch_2.operator()<C0, cmp_type::none>();
                        break;
                    case cmp_type::masked:
                        dispatch_2.operator()<C0, cmp_type::masked>();
                        break;
                    case cmp_type::full:
                        dispatch_2.operator()<C0, cmp_type::full>();
                        break;
                }
            };

            if (c0_masked)
                dispatch_1.operator()<cmp_type::masked>();
            else
                dispatch_1.operator()<cmp_type::full>();
        }

        return result;
    }
}
