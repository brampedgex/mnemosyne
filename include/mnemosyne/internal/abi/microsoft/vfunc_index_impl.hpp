#pragma once
#include "../../../defines.hpp"

#include <array>
#include <utility>
#include <cstdint>
#include <cstddef>

#ifdef MNEMOSYNE_ABI_MICROSOFT

namespace mnem::internal {
    template <size_t N>
    struct fake_func {
        static size_t fn() {
            return N;
        }
    };

    template <class>
    struct fake_vtable;

    template <size_t... Idxs>
    struct fake_vtable<std::integer_sequence<size_t, Idxs...>> {
        using T = std::integer_sequence<size_t, Idxs...>;
        std::array<size_t(*)(), T::size()> funcs = std::to_array({ &fake_func<Idxs>::fn... });

        static inline constinit fake_vtable<std::integer_sequence<size_t, Idxs...>> instance{};
        static inline constinit void* obj = &instance;
    };

    template <class F, class T>
    inline size_t vfunc_index_impl(F T::*fn) {
        // Unfortunately we can't tell if fn isn't actually a virtual thunk
        // TODO: if (sizeof(fn) > sizeof(uintptr_t)) throw std::runtime_error("...");
        union {
            F T::*func;
            void* ptr;
        };

        func = fn;
        return reinterpret_cast<size_t(*)(void**)>(ptr)(&fake_vtable<std::make_index_sequence<512>>::obj);
    }
}

#endif