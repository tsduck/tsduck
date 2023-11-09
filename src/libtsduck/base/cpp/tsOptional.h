//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Additional features for the C++17 standard std::optional template class.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsPlatform.h"

//
// If C++17 is available, use the predefined implementation.
// Otherwise, declare a custom version of std::optional.
// Reference: https://en.cppreference.com/w/cpp/utility/optional
//
#if defined(TS_CXX17)
#include <optional>
#elif !defined(DOXYGEN)

// Identify TSDuck implementation of std::optional before C++17.
#define TS_OPTIONAL_IMPLEMENTED 1

namespace std {

    struct in_place_t {
        explicit in_place_t() = default;
    };
    constexpr in_place_t in_place{};

    struct nullopt_t {
        nullopt_t() = delete;
        explicit constexpr nullopt_t(int) noexcept {}
    };
    constexpr nullopt_t nullopt{0};

    // On macOS, bad_optional_access is defined before C++17.
#if !defined(TS_MAC)
    TS_PUSH_WARNING()
    TS_GCC_NOWARNING(weak-vtables)
    TS_LLVM_NOWARNING(weak-vtables)
    class bad_optional_access: public exception
    {
    public:
        bad_optional_access() = default;
        virtual ~bad_optional_access() override = default;
        const char* what() const noexcept override { return "bad optional access"; }
    };
    TS_POP_WARNING()
#endif

    template <class T>
    class optional
    {
    private:
        T* _access = nullptr;     // point to _data when initialized
        uint8_t _data[sizeof(T)]; // flat memory area for T instance
    public:

        explicit operator bool() const noexcept { return _access != nullptr; }
        bool has_value() const noexcept { return _access != nullptr; }

        void swap(optional& other);

        void reset();
        ~optional() { reset(); }

        optional() noexcept = default;
        optional(std::nullopt_t) noexcept {}
        optional(const T& value) : _access(new(_data) T(value)) {}
        optional(T&& value) : _access(new(_data) T(std::move(value))) {}

        optional(const optional& other)
        {
            if (other._access != nullptr) {
                _access = new(_data) T(*(other._access));
            }
        }

        optional(optional&& other) noexcept
        {
            if (other._access != nullptr) {
                _access = new(_data) T(std::move(*(other._access)));
                other._access = nullptr;
            }
        }

        template <class... Args>
        explicit optional(std::in_place_t, Args&&... args) :
            _access(new(_data) T(std::forward<Args>(args)...)) {}

        template <class U, class... Args>
        constexpr explicit optional(std::in_place_t, std::initializer_list<U> ilist, Args&&... args) :
            _access(new(_data) T(ilist, std::forward<Args>(args)...)) {}

        optional& operator=(std::nullopt_t) noexcept
        {
            reset();
            return *this;
        }

        optional& operator=(const optional& other)
        {
            if (&other != this) {
                reset();
                if (other._access != nullptr) {
                    _access = new(_data) T(*(other._access));
                }
            }
            return *this;
        }

        optional& operator=(optional&& other)
        {
            if (&other != this) {
                reset();
                if (other._access != nullptr) {
                    _access = new(_data) T(std::move(*(other._access)));
                    other._access = nullptr;
                }
            }
            return *this;
        }

        template <class U>
        optional& operator=(const U& value)
        {
            reset();
            _access = new(_data) T(value);
            return *this;
        }

        const T* operator->() const { return _access; }
        T* operator->() { return _access; }

        const T& operator*() const& noexcept { return *_access; }
        T& operator*() & noexcept { return *_access; }

        T&& operator*() && noexcept
        {
            T* tmp = _access;
            _access = nullptr;
            return std::move(*tmp);
        }

        T& value() &
        {
            if (_access != nullptr) {
                return *_access;
            }
            else {
                throw bad_optional_access();
            }
        }

        const T& value() const&
        {
            if (_access != nullptr) {
                return *_access;
            }
            else {
                throw bad_optional_access();
            }
        }

        T&& value() &&
        {
            if (_access != nullptr) {
                T* tmp = _access;
                _access = nullptr;
                return std::move(*tmp);
            }
            else {
                throw bad_optional_access();
            }
        }

        template <class U>
        T value_or(const U& default_value) const&
        {
            if (_access != nullptr) {
                return *_access;
            }
            else {
                return static_cast<T>(default_value);
            }
        }

        template <class... Args>
        T& emplace(Args&&... args)
        {
            reset();
            return *(_access = new(_data) T(std::forward<Args>(args)...));
        }

        template <class U, class... Args>
        T& emplace(std::initializer_list<U> ilist, Args&&... args)
        {
            reset();
            return *(_access = new(_data) T(ilist, std::forward<Args>(args)...));
        }
    };

    template <class T>
    void swap(optional<T>& lhs, optional<T>& rhs) noexcept
    {
        lhs.swap(rhs);
    }

    template <class T, class U>
    constexpr bool operator==(const optional<T>& lhs, const optional<U>& rhs)
    {
        return bool(lhs) == bool(rhs) && (!lhs || *lhs == *rhs);
    }

    template <class T, class U>
    constexpr bool operator!=(const optional<T>& lhs, const optional<U>& rhs)
    {
        return !(lhs == rhs);
    }

    template <class T, class U>
    constexpr bool operator<(const optional<T>& lhs, const optional<U>& rhs)
    {
        return bool(rhs) && (!lhs || *lhs < *rhs);
    }

    template <class T, class U>
    constexpr bool operator<=(const optional<T>& lhs, const optional<U>& rhs)
    {
        return !lhs || (bool(rhs) && *lhs <= *rhs);
    }

    template <class T, class U>
    constexpr bool operator>(const optional<T>& lhs, const optional<U>& rhs)
    {
        return rhs < lhs;
    }

    template <class T, class U>
    constexpr bool operator>=(const optional<T>& lhs, const optional<U>& rhs)
    {
        return rhs <= lhs;
    }

    template <class T>
    constexpr bool operator==(const optional<T>& opt, std::nullopt_t) noexcept
    {
        return !opt;
    }

    template <class T>
    constexpr bool operator==(std::nullopt_t, const optional<T>& opt) noexcept
    {
        return !opt;
    }

    template <class T>
    constexpr bool operator!=(const optional<T>& opt, std::nullopt_t) noexcept
    {
        return bool(opt);
    }

    template <class T>
    constexpr bool operator!=(std::nullopt_t, const optional<T>& opt) noexcept
    {
        return bool(opt);
    }

    template <class T>
    constexpr bool operator<(const optional<T>& opt, std::nullopt_t) noexcept
    {
        return false;
    }

    template <class T>
    constexpr bool operator<(std::nullopt_t, const optional<T>& opt) noexcept
    {
        return bool(opt);
    }

    template <class T>
    constexpr bool operator<=(const optional<T>& opt, std::nullopt_t) noexcept
    {
        return !opt;
    }

    template <class T>
    constexpr bool operator<=(std::nullopt_t, const optional<T>& opt) noexcept
    {
        return true;
    }

    template <class T>
    constexpr bool operator>(const optional<T>& opt, std::nullopt_t) noexcept
    {
        return bool(opt);
    }

    template <class T>
    constexpr bool operator>(std::nullopt_t, const optional<T>& opt) noexcept
    {
        return false;
    }

    template <class T>
    constexpr bool operator>=(const optional<T>& opt, std::nullopt_t) noexcept
    {
        return true;
    }

    template <class T>
    constexpr bool operator>=(std::nullopt_t, const optional<T>& opt) noexcept
    {
        return !opt;
    }

    template <class T, class U>
    constexpr bool operator==(const optional<T>& opt, const U& value)
    {
        return bool(opt) && opt.value() == static_cast<T>(value);
    }

    template <class T, class U>
    constexpr bool operator==(const T& value, const optional<U>& opt)
    {
        return opt == value;
    }

    template <class T, class U>
    constexpr bool operator!=(const optional<T>& opt, const U& value)
    {
        return !(opt == value);
    }

    template <class T, class U>
    constexpr bool operator!=(const T& value, const optional<U>& opt)
    {
        return !(opt == value);
    }

    template <class T, class U>
    constexpr bool operator<(const optional<T>& opt, const U& value)
    {
        return !opt || *opt < value;
    }

    template <class T, class U>
    constexpr bool operator<(const T& value, const optional<U>& opt)
    {
        return bool(opt) && value < *opt;
    }

    template <class T, class U>
    constexpr bool operator<=(const optional<T>& opt, const U& value)
    {
        return !opt || *opt <= value;
    }

    template <class T, class U>
    constexpr bool operator<=(const T& value, const optional<U>& opt)
    {
        return bool(opt) && value <= *opt;
    }

    template <class T, class U>
    constexpr bool operator>(const optional<T>& opt, const U& value)
    {
        return bool(opt) && *opt > value;
    }

    template <class T, class U>
    constexpr bool operator>(const T& value, const optional<U>& opt)
    {
        return !opt || *opt <= value;
    }

    template <class T, class U>
    constexpr bool operator>=(const optional<T>& opt, const U& value)
    {
        return bool(opt) && *opt >= value;
    }

    template <class T, class U>
    constexpr bool operator>=(const T& value, const optional<U>& opt)
    {
        return !opt || value >= *opt;
    }

    template <class T>
    constexpr optional<typename decay<T>::type> make_optional(T&& value)
    {
        return std::optional<typename decay<T>::type>(std::forward<T>(value));
    }

    template <class T, class... Args>
    constexpr optional<T> make_optional(Args&&... args)
    {
        return std::optional<T>(std::in_place, std::forward<Args>(args)...);
    }

    template <class T, class U, class... Args>
    constexpr optional<T> make_optional(initializer_list<U> il, Args&&... args)
    {
        return std::optional<T>(std::in_place, il, std::forward<Args>(args)...);
    }
}

template <class T>
void std::optional<T>::reset()
{
    if (_access != nullptr) {
        T* tmp = _access;
        _access = nullptr; // _access is null even in case of exception in T destructor
        tmp->~T();
    }
}

template <class T>
void std::optional<T>::swap(optional& other)
{
    if (_access != nullptr && other._access != nullptr) {
        std::swap(*_access, *other._access);
    }
    else if (_access != nullptr) {
        other._access = new(_data) T(std::move(*(_access)));
        _access = nullptr;

    }
    else if (other._access != nullptr) {
        _access = new(_data) T(std::move(*(other._access)));
        other._access = nullptr;
    }
}
#endif // TS_CXX17

namespace ts {
    //!
    //! Set a default value in a std::optional object, if there is none.
    //! @tparam T The type of the optional object.
    //! @tparam U The type of the default value to set.
    //! @param [in,out] opt The optinal object to set.
    //! @param [in] value The value to set in @a obj if it is not initialized.
    //!
    template <class T, class U>
    inline void set_default(std::optional<T>& opt, const U& value)
    {
        if (!opt) {
            opt = value;
        }
    }
}
