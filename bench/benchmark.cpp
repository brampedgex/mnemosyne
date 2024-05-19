#include <iostream>
#include <string_view>
#include <random>
#include <numeric>
#include <chrono>

#include <mnemosyne/scan/scanner.hpp>

static std::string_view mode_to_string(mnem::scan_mode mode) {
    switch (mode) {
        case mnem::scan_mode::normal:
            return "normal";
        case mnem::scan_mode::sse4_2:
            return "SSE4.2";
        case mnem::scan_mode::avx2:
            return "AVX2";
        case mnem::scan_mode::avx512:
            return "AVX512";
        default:
            return "<unknown>";
    }
}

static void benchmark(mnem::scan_mode mode, mnem::scan_align align) {
    static constexpr size_t max_size = 0x40000000; // 1G, TODO a way to change this

    for (size_t size = 16; size <= max_size; size <<= 1) {
        std::cout << mode_to_string(mode) << " scanner with " << size << " byte buffer... ";

        auto sig = mnem::make_signature<"01 02 03 04 05 06 07 08 09 10 11 12 13 14 15 16">();

        const size_t iterations = std::min(max_size * 40 / size, {0x100000});
        const int buffer_count = 8; // multiple buffers since the allocator can seem to cause the results to vary wildly

        long long totalTime = 0;
        for (int i = 0; i < buffer_count; i++) {
            auto buffer = std::make_unique<std::byte[]>(size);

            // Random data that is consistent across all machines
            std::mt19937 rng; // NOLINT
            std::uniform_int_distribution<uint64_t> dist{ 0, std::numeric_limits<uint64_t>::max() };
            for (auto ptr = reinterpret_cast<uint64_t*>(buffer.get()); ptr < reinterpret_cast<uint64_t*>(buffer.get() + size); ptr++)
                *ptr = dist(rng);

            mnem::scanner scanner{ mnem::memory_span{ buffer.get(), size }, mode };
            const size_t itersPerBuffer = iterations / buffer_count;

            auto start = std::chrono::steady_clock::now();
            for (int j = 0; j < itersPerBuffer; j++)
                [[maybe_unused]] auto _ = scanner.scan_signature(sig, align);
            auto end = std::chrono::steady_clock::now();

            totalTime += std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
        }

        auto us_per_iter = static_cast<double>(totalTime) / static_cast<double>(iterations);
        // bytes per microsecond = megabytes per second
        auto mbps = static_cast<double>(size) / us_per_iter;

        std::cout << '~' << us_per_iter / 1000.0 << "ms per scan, " << mbps << " MB/s\n";
    }
}

int main() {
    auto fastest_mode = mnem::detect_scan_mode();
    std::cout << "Highest supported mode: " << mode_to_string(fastest_mode) << '\n';

    benchmark(mnem::scan_mode::avx2, mnem::scan_align::x16);

    //for (int mode = static_cast<int>(mnem::scan_mode::normal); mode <= static_cast<int>(fastest_mode); mode++) {
    //    benchmark(static_cast<mnem::scan_mode>(mode));
    //}
}
