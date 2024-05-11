#pragma once
#ifndef MNEMOSYNE_MEM_WRAPPER_HPP
#define MNEMOSYNE_MEM_WRAPPER_HPP

#include <algorithm>
#include <cstddef>

namespace mnem {
    struct no_construct_t {};

    /// Tag used to prevent the initialization of a wrap<T> object.
    constexpr no_construct_t no_construct;

    /// Controls the constructor behavior of wrap<T> objects.
    template <class T>
    struct wrapper_traits {
        static void construct(std::byte* buffer, auto&&... args) {
            new(buffer) T(std::forward<decltype(args)>(args)...);
        }

        static void destroy(T* obj) {
            obj->~T();
        }
    };

    /// General-purpose wrapper for using custom constructors/destructors.
    /// Prevents devirtualization and gives control over constructors/destructors with the wrapper_traits class.
    template <class T>
    class wrap {
    public:
        explicit wrap(no_construct_t) {}

        ~wrap() {
            wrapper_traits<T>::destroy(this->get());
        }

        wrap() {
            wrapper_traits<T>::construct(this->buffer());
        }

        wrap(const wrap<T>& copy) {
            wrapper_traits<T>::construct(this->buffer(), *copy);
        }

        wrap(wrap<T>&& move) noexcept(false) {
            wrapper_traits<T>::construct(this->buffer(), *std::move(move));
        }

        explicit wrap(std::in_place_t, auto&&... args) {
            wrapper_traits<T>::construct(this->buffer(), std::forward<decltype(args)>(args)...);
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
}

#endif
