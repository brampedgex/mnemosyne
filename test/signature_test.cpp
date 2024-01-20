#include <gtest/gtest.h>
#include <mnemosyne/scan/signature.hpp>

TEST(MnemosyneTests, SignatureTest) {
    using namespace mnem::sig_literals;

    // Testing compile time parse and each letter
    {
        static constexpr auto sig1 = mnem::parse_static_signature<"01 23 45 67 89 AB CD EF">();
        auto& c = sig1.container();

        ASSERT_EQ(c[0].byte(), std::byte{0x01});
        ASSERT_EQ(c[1].byte(), std::byte{0x23});
        ASSERT_EQ(c[2].byte(), std::byte{0x45});
        ASSERT_EQ(c[3].byte(), std::byte{0x67});
        ASSERT_EQ(c[4].byte(), std::byte{0x89});
        ASSERT_EQ(c[5].byte(), std::byte{0xAB});
        ASSERT_EQ(c[6].byte(), std::byte{0xCD});
        ASSERT_EQ(c[7].byte(), std::byte{0xEF});

        for (auto& i : c)
            ASSERT_EQ(i.mask(), std::byte{0xFF});
    }

    // Testing string literals, wildcards, and syntax edge cases
    {
        static constexpr auto sig2 = "1 2? ?4 ? ?? 9A BCDE F"_sig;
        auto& c = sig2.container();

        // FIXME: Super ugly, don't know another way to do this
        ASSERT_EQ(c[0].byte(), std::byte{0x01});
        ASSERT_EQ(c[0].mask(), std::byte{0xFF});
        ASSERT_EQ(c[1].byte(), std::byte{0x20});
        ASSERT_EQ(c[1].mask(), std::byte{0xF0});
        ASSERT_EQ(c[2].byte(), std::byte{0x04});
        ASSERT_EQ(c[2].mask(), std::byte{0x0F});
        ASSERT_EQ(c[3].byte(), std::byte{0x00});
        ASSERT_EQ(c[3].mask(), std::byte{0x00});
        ASSERT_EQ(c[4].byte(), std::byte{0x00});
        ASSERT_EQ(c[4].mask(), std::byte{0x00});
        ASSERT_EQ(c[5].byte(), std::byte{0x9A});
        ASSERT_EQ(c[5].mask(), std::byte{0xFF});
        ASSERT_EQ(c[6].byte(), std::byte{0xBC});
        ASSERT_EQ(c[6].mask(), std::byte{0xFF});
        ASSERT_EQ(c[7].byte(), std::byte{0xDE});
        ASSERT_EQ(c[7].mask(), std::byte{0xFF});
        ASSERT_EQ(c[8].byte(), std::byte{0x0F});
        ASSERT_EQ(c[8].mask(), std::byte{0xFF});
    }
}