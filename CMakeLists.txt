cmake_minimum_required(VERSION 3.22)
project(mnemosyne)

set(CMAKE_CXX_STANDARD 20)

add_library(mnemosyne
        include/mnemosyne/core/memory_range.hpp
        include/mnemosyne/scan/signature.hpp
        include/mnemosyne/internal/string_literal.hpp
        include/mnemosyne/scan/scanner.hpp
        src/mnemosyne.cpp
        include/mnemosyne/mem/process.hpp
        src/scan/scanner.cpp
        src/scan/scanner_impls.hpp
        src/scan/scanner_avx2.cpp)

if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    set_source_files_properties(src/scan/scanner_avx2.cpp PROPERTIES COMPILE_FLAGS "-mavx -mavx2 -mbmi")
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    set_source_files_properties(src/scan/scanner_avx2.cpp PROPERTIES COMPILE_FLAGS "/arch:AVX2")
endif()

if (WIN32)
    target_sources(mnemosyne PRIVATE src/os/win32/process.cpp)
elseif(LINUX)
    target_sources(mnemosyne PRIVATE src/os/linux/process.cpp)
else()
    message(SEND_ERROR "Mnemosyne does not support the current platform.")
endif()

add_subdirectory(test)
add_subdirectory(bench)

target_include_directories(mnemosyne PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>)
