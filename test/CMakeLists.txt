include(FetchContent)

FETCHCONTENT_DECLARE(
        googletest
        GIT_REPOSITORY  https://github.com/google/googletest
        GIT_TAG         main
        FIND_PACKAGE_ARGS NAMES gtest_main
)

FETCHCONTENT_MAKEAVAILABLE(googletest)

add_executable(MnemosyneTests
        signature_test.cpp scanner_test.cpp vfunc_index_test.cpp)

target_link_libraries(MnemosyneTests PUBLIC gtest_main mnemosyne)

add_test(NAME MnemosyneTests COMMAND MnemosyneTests)
