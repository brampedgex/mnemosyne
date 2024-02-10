#include <mnemosyne/mem/process.hpp>
#include <link.h>
#include "elf64.hpp"

auto mnem::proc_module::get_memory_range() const -> memory_span {
    // TODO: this code is EXTREMELY sus bc i stole it from a linux csgo cheat
    auto* info = reinterpret_cast<dl_phdr_info*>(this->ptr);
    return { reinterpret_cast<std::byte*>(info->dlpi_addr + info->dlpi_phdr[0].p_vaddr), info->dlpi_phdr[0].p_memsz };
}

auto mnem::proc_module::get_section_range(std::string_view name) const -> std::optional<memory_span> {
    auto* info = reinterpret_cast<dl_phdr_info*>(this->ptr);
    elf64_ehdr* pelf64_ehdr = nullptr;

    for(int i = 0; i < info->dlpi_phnum; i++){
        const ElfW(Phdr)* phdr = &info->dlpi_phdr[i];
        if(phdr->p_type == PT_PHDR){
            pelf64_ehdr = reinterpret_cast<elf64_ehdr*>(info->dlpi_addr + phdr->p_offset - phdr->p_vaddr);
            break;
        }
    }   

    elf64_shdr* pelf64_shdr = reinterpret_cast<elf64_shdr*>(info->dlpi_addr + pelf64_ehdr->e_shoff);

    for(uint32_t i = 0; i < pelf64_ehdr->e_shnum; i++){
        const char* section_name = (const char*)(info->dlpi_addr + pelf64_shdr[pelf64_ehdr->e_shstrndx].sh_offset + pelf64_shdr[i].sh_name);
        std::string_view sv_section_name(section_name);
        if(name == sv_section_name){
            memory_span section_rng((std::byte*)pelf64_shdr[i].sh_addr, pelf64_shdr[i].sh_size); //idk if the size is the section size or section start addr + size 
            return section_rng;
        }
    }

    return std::nullopt;
}

auto mnem::get_main_proc_module() -> proc_module {
    // TODO: how
}

auto mnem::get_proc_module(const std::string& name) -> std::optional<proc_module> {
    // TODO: lmao i need a lambda wrapper cuz this is STUPID
    struct silly_struct {
        dl_phdr_info* result = nullptr;
        const std::string* name = nullptr;
    } silly { .name = &name };

    dl_iterate_phdr([](dl_phdr_info* info, size_t, void* param) {
        auto& silly = *reinterpret_cast<silly_struct*>(param);

        if (*silly.name == info->dlpi_name) {
            silly.result = info;
            return 1;
        }

        return 0;
    }, &silly);

    return silly.result ? std::make_optional(proc_module{ silly.result }) : std::nullopt;
}
