#include <gtest/gtest.h>
#include <mnemosyne/scan/scanner.hpp>

#include <random>

static constexpr size_t BUFFER_SIZE = 0x100000; // 1MiB

namespace {
    auto make_buffer() {
        auto buffer = std::make_unique<std::byte[]>(BUFFER_SIZE);

        // Random data that is consistent across all machines
        std::mt19937 rng; // NOLINT
        std::uniform_int_distribution<uint64_t> dist{0, std::numeric_limits<uint64_t>::max()};
        for (auto ptr = reinterpret_cast<uint64_t *>(buffer.get());
             ptr < reinterpret_cast<uint64_t *>(buffer.get() + BUFFER_SIZE); ptr++)
            *ptr = dist(rng);

        return buffer;
    }

    void write_sig(std::byte* dest, mnem::signature sig) {
        for (auto& i : sig) {
            *dest++ = i.byte();
        }
    }

    void write_decoy(std::byte* dest, mnem::signature sig) {
        for (auto& i : sig) {
            *dest++ = i.byte();
        }
        dest--;
        *dest = *dest ^ std::byte{0xFF}; // flip the last bits
    }
}

TEST(MnemosyneTests, ScannerTest) {
    // TODO: Overhaul this to cover more edge cases of each scanner.

    static constexpr size_t EXPECTED_OFFSET = 39683; // random number i made by mashing my keyboard (it's not some genius magic value)

    int highest_mode = static_cast<int>(mnem::detect_scan_mode());

    for (int i = static_cast<int>(mnem::scan_mode::normal); i <= highest_mode; i++) {
        auto buffer = make_buffer();

        buffer[EXPECTED_OFFSET + 0x00] = std::byte{0x00};
        buffer[EXPECTED_OFFSET + 0x01] = std::byte{0x01};
        buffer[EXPECTED_OFFSET + 0x02] = std::byte{0x02};
        buffer[EXPECTED_OFFSET + 0x03] = std::byte{0x03};
        buffer[EXPECTED_OFFSET + 0x04] = std::byte{0x04};
        buffer[EXPECTED_OFFSET + 0x05] = std::byte{0x05};

        mnem::memory_span range{ buffer.get(), BUFFER_SIZE };
        mnem::scanner scanner{ range, static_cast<mnem::scan_mode>(i) };

        auto result = scanner.scan_signature(mnem::make_signature<"00 01 02 03 04 05">());
        EXPECT_NE(result, nullptr);

        if (result) {
            auto offset = result - buffer.get();
            EXPECT_EQ(offset, EXPECTED_OFFSET);
        }
    }
}
