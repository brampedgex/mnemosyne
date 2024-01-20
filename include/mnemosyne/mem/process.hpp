#pragma once
#ifndef MNEMOSYNE_MEM_PROCESS_HPP
#define MNEMOSYNE_MEM_PROCESS_HPP

#include <string_view>
#include <optional>

#include "../core/memory_range.hpp"

namespace mnem {
    struct proc_module {
        void* ptr;

        memory_span get_memory_range();
        std::optional<memory_span> get_section_range(std::string_view name);
    };

    proc_module get_main_proc_module();
    proc_module get_proc_module(const std::string& name);
}

#endif