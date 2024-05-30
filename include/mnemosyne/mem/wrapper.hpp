#pragma once
#ifndef MNEMOSYNE_MEM_WRAPPER_HPP
#define MNEMOSYNE_MEM_WRAPPER_HPP

#include <algorithm>
#include <cstddef>

namespace mnem {
    struct no_construct_t {};

    /// Tag used to prevent the initialization of a wrap<T> object.
    constexpr no_construct_t no_construct;

    /// Provides a default destructor 
    template <class T>
    struct wrapper_default_destroy {
        static void destroy(T* obj) requires requires { obj->~T(); } {
            obj->~T();
        }
    };

    /// Controls the constructor behavior of wrap<T> objects.
    template <class T>
    struct wrapper_traits : wrapper_default_destroy<T> {
        static void construct(std::byte* buffer, auto&&... args) requires requires { new(buffer) T(std::forward<decltype(args)>(args)...); } {
            new(buffer) T(std::forward<decltype(args)>(args)...);
        }
    };

    /// General-purpose wrapper for using custom constructors/destructors.
    /// Prevents devirtualization and gives control over constructors/destructors with the wrapper_traits class.
    template <class T, class Traits = wrapper_traits<T>>
    requires requires(T* obj) { Traits::destroy(obj); }
    class wrap {
    public:
        explicit wrap(no_construct_t) {}

        ~wrap() {
            Traits::destroy(this->get());
        }

        wrap() = delete;
        wrap() requires requires(std::byte* buf) { Traits::construct(buf); } {
            Traits::construct(this->buffer());
        }

        wrap(const wrap<T, Traits>&) = delete;
        wrap(const wrap<T, Traits>& copy) requires requires(std::byte* buf, const T& obj) { Traits::construct(buf, obj); } {
            Traits::construct(this->buffer(), *copy);
        }

        wrap(wrap<T, Traits>&&) = delete;
        wrap(wrap<T, Traits>&& move) requires requires(std::byte* buf, T&& obj) { Traits::construct(buf, std::move(obj)); } {
            Traits::construct(this->buffer(), *std::move(move));
        }

        template <class Traits2>
        requires requires(std::byte* buf, const T& obj) { Traits::construct(buf, obj); }
        wrap(const wrap<T, Traits2>& copy) {
            Traits::construct(this->buffer(), *copy);
        }

        template <class Traits2>
        requires requires(std::byte* buf, T&& obj) { Traits::construct(buf, std::move(obj)); }
        wrap(wrap<T, Traits2>&& move) noexcept(false) {
            Traits::construct(this->buffer(), *std::move(move));
        }

        explicit wrap(std::in_place_t, auto&&... args) requires requires { Traits::construct(this->buffer(), std::forward<decltype(args)>(args)...); } {
            Traits::construct(this->buffer(), std::forward<decltype(args)>(args)...);
        }

        /// Destroys the existing underlying value and constructs a new value in-place.
        void emplace(auto&&... args) requires requires { Traits::construct(this->buffer(), std::forward<decltype(args)>(args)...); } {
            Traits::destroy(this->get());
            Traits::construct(this->buffer(), std::forward<decltype(args)>(args)...);
        }

        [[nodiscard]] std::byte* buffer() {
            return this->buffer_;
        }

        [[nodiscard]] const std::byte* buffer() const {
            return this->buffer_;
        }

        [[nodiscard]] auto* get() {
            return std::launder(reinterpret_cast<T*>(this->buffer_));
        }

        [[nodiscard]] auto* get() const {
            return std::launder(reinterpret_cast<const T*>(this->buffer_));
        }

        [[nodiscard]] auto& operator*() & {
            return *this->get();
        }

        [[nodiscard]] auto& operator*() const& {
            return *this->get();
        }

        [[nodiscard]] auto&& operator*() && {
            return std::move(*this->get());
        }

        [[nodiscard]] auto&& operator*() const&& {
            return std::move(*this->get());
        }

        [[nodiscard]] T* operator->() {
            return this->get();
        }

        [[nodiscard]] const T* operator->() const {
            return this->get();
        }

    private:
        alignas(alignof(T)) std::byte buffer_[sizeof(T)]{};
    };

    template <class T, class Traits = wrapper_traits<T>>
    requires std::move_constructible<wrap<T, Traits>>
    auto make_wrap(auto&&... args) {
        return wrap<T, Traits>{ std::in_place, std::forward<decltype(args)>(args)... };
    }
}

#endif
