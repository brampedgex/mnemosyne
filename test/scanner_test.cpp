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
    mnem::memory_span range{ reinterpret_cast<std::byte*>(bytes), 18 };
    mnem::scanner scanner{ range };

    auto offset = scanner.scan_signature(mnem::make_signature<"3A 5 ?? 71">()) - reinterpret_cast<std::byte*>(bytes);
    ASSERT_EQ(offset, 11);
}
