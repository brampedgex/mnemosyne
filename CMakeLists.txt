cmake_minimum_required(VERSION 3.25)
project(mnemosyne)

set(CMAKE_CXX_STANDARD 20)

add_library(mnemosyne include/mnemosyne/core/memory_range.hpp include/mnemosyne/scan/signature.hpp include/mnemosyne/internal/string_literal.hpp include/mnemosyne/scan/scanner.hpp src/mnemosyne.cpp include/mnemosyne/mem/process.hpp)

if (WIN32)
    target_sources(mnemosyne PRIVATE src/os/win32/process.cpp)
endif()

add_subdirectory(test)

target_include_directories(mnemosyne PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>)
