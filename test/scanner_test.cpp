#include <gtest/gtest.h>
#include <mnemosyne/scan/scanner.hpp>

uint8_t bytes[] = {
        0x00,
        0x11,
        0x22,
        0x3A,
        0x05,
        0xFF,
        0x70,
        0x00,
        0x00,
        0x50,
        0x01,
        0x3A, // <-- expected result
        0x05,
        0xF0,
        0x71,
        0x00,
        0x50,
        0x11
};

TEST(MnemosyneTests, ScannerTest) {
    mnem::memory_range range{ reinterpret_cast<std::byte*>(bytes), reinterpret_cast<std::byte*>(&bytes[18]) };
    mnem::scanner scanner{ range };

    auto result = scanner.scan_signature(mnem::make_signature<"3A 5 ?? 71">());
    size_t idx = reinterpret_cast<uint8_t*>(result) - bytes;
    ASSERT_EQ(idx, 11);
}