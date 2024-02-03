#include <gtest/gtest.h>
#include <mnemosyne/core/vfunc_index.hpp>

struct foo {
    virtual void a() = 0; // 0
    virtual void b() = 0; // 1
    virtual void c() = 0; // 2
};

TEST(MnemosyneTests, VFuncIndexTest) {
    ASSERT_EQ(mnem::vfunc_index(&foo::a), 0);
    ASSERT_EQ(mnem::vfunc_index(&foo::b), 1);
    ASSERT_EQ(mnem::vfunc_index(&foo::c), 2);
}