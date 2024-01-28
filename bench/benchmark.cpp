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

static void benchmark(mnem::scan_mode mode) {
    static constexpr size_t max_size = 0x40000000; // 1G, TODO a way to change this

    for (size_t size = 16; size <= max_size; size <<= 1) {
        auto buffer = std::make_unique<std::byte[]>(size);

        // Random data that is consistent across all machines
        std::mt19937 rng; // NOLINT
        std::uniform_int_distribution<uint64_t> dist{ 0, std::numeric_limits<uint64_t>::max() };
        for (auto ptr = reinterpret_cast<uint64_t*>(buffer.get()); ptr < reinterpret_cast<uint64_t*>(buffer.get() + size); ptr++)
            *ptr = dist(rng);

        std::cout << mode_to_string(mode) << " scanner with " << size << " byte buffer... ";

        mnem::scanner scanner{ mnem::memory_span{ buffer.get(), size } };
        auto sig = mnem::make_signature<"01 02 03 04 05 06 07 08 09 10 11 12 13 14 15 16">();

        const size_t iterations = std::min(max_size * 20 / size, {1'000'000});

        auto start = std::chrono::steady_clock::now();
        for (int i = 0; i < iterations; i++)
            [[maybe_unused]] auto _ = scanner.scan_signature(sig, mode);
        auto end = std::chrono::steady_clock::now();

        auto us_per_iter = static_cast<double>(std::chrono::duration_cast<std::chrono::microseconds>(end - start).count()) / static_cast<double>(iterations);
        // bytes per microsecond = megabytes per second
        auto mbps = static_cast<double>(size) / us_per_iter;

        std::cout << '~' << us_per_iter / 1000.0 << "ms per scan, " << mbps << " MB/s\n";
    }
}

int main() {
    auto fastest_mode = mnem::detect_scan_mode();
    std::cout << "Highest supported mode: " << mode_to_string(fastest_mode) << '\n';

    benchmark(mnem::scan_mode::avx2);

    //for (int mode = static_cast<int>(mnem::scan_mode::normal); mode <= static_cast<int>(fastest_mode); mode++) {
    //    benchmark(static_cast<mnem::scan_mode>(mode));
    //}
}