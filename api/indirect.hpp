/*
 * Copyright (c) 2016 Joseph Thomson
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#pragma once

#ifndef INDIRECT_HPP
#define INDIRECT_HPP

#include <optional.hpp>
#include <type_traits.hpp>

#include <functional>
#include <iosfwd>
#include <stdexcept>
#include <utility>

//======================
// forward declarations
//======================

template <typename>
class indirect;

template <typename>
class optional_indirect;

template <typename T>
constexpr T* get_pointer(indirect<T> const&) noexcept;

template <typename T>
constexpr T* get_pointer(indirect<T>&) noexcept;

template <typename T>
constexpr T* get_pointer(optional_indirect<T> const&) noexcept;

template <typename T>
constexpr T* get_pointer(optional_indirect<T>&) noexcept;

//==========
// indirect
//==========

template <typename T>
class indirect
{
public:
    using value_type = T;
    using const_type = indirect<std::add_const_t<T>>;

private:
    T* target;

    template <typename U>
    static constexpr U* throw_if_null(U* p)
    {
        if (!p)
        {
            throw std::invalid_argument("Cannot convert null pointer to indirect.");
        }

        return p;
    }

public:
    constexpr indirect(T& r) noexcept :
        target(&r)
    {
    }

    constexpr explicit indirect(T* p) :
        target(throw_if_null(p))
    {
    }

    template <typename U, typename = std::enable_if_t<std::is_convertible<U*, T*>::value>>
    constexpr indirect(indirect<U> const& v) noexcept :
        target(get_pointer(v))
    {
    }

    template <typename U, typename = std::enable_if_t<std::is_convertible<U*, T*>::value>>
    constexpr explicit indirect(optional_indirect<U> const& v) :
        target(throw_if_null(get_pointer(v)))
    {
    }

    constexpr T& operator*() const noexcept
    {
        return *target;
    }

    constexpr T* operator->() const noexcept
    {
        return target;
    }

    constexpr explicit operator T*() const noexcept
    {
        return target;
    }

    void swap(indirect& other) noexcept
    {
        using std::swap;
        swap(target, other.target);
    }
};

template <typename T>
constexpr indirect<T> make_indirect(T& r) noexcept
{
    return r;
}

template <typename T, typename U>
constexpr indirect<T> static_indirect_cast(indirect<U> const& v) noexcept
{
    return static_cast<T&>(*v);
}

template <typename T, typename U>
constexpr indirect<T> dynamic_indirect_cast(indirect<U> const& v)
{
    return dynamic_cast<T&>(*v);
}

template <typename T, typename U>
constexpr indirect<T> const_indirect_cast(indirect<U> const& v) noexcept
{
    return const_cast<T&>(*v);
}

template <typename T>
constexpr T* get_pointer(indirect<T> const& v) noexcept
{
    return static_cast<T*>(v);
}

template <typename T>
constexpr T* get_pointer(indirect<T>& v) noexcept
{
    return static_cast<T*>(v);
}

template <typename T>
void swap(indirect<T>& lhs, indirect<T>& rhs)
{
    lhs.swap(rhs);
}

template <typename T1, typename T2>
constexpr bool operator==(indirect<T1> const& lhs, indirect<T2> const& rhs) noexcept
{
    return get_pointer(lhs) == get_pointer(rhs);
}

template <typename T1, typename T2, std::enable_if_t<std::is_convertible<T1*, T2*>::value || std::is_convertible<T2*, T1*>::value, int> = 0>
constexpr bool operator==(indirect<T1> const& lhs, T2& rhs) noexcept
{
    return get_pointer(lhs) == &rhs;
}

template <typename T1, typename T2, std::enable_if_t<std::is_convertible<T1*, T2*>::value || std::is_convertible<T2*, T1*>::value, int> = 0>
constexpr bool operator==(T1& lhs, indirect<T2> const& rhs) noexcept
{
    return &lhs == get_pointer(rhs);
}

template <typename T1, typename T2>
constexpr bool operator!=(indirect<T1> const& lhs, indirect<T2> const& rhs) noexcept
{
    return !(lhs == rhs);
}

template <typename T1, typename T2>
constexpr bool operator!=(indirect<T1> const& lhs, T2 const& rhs) noexcept
{
    return !(lhs == rhs);
}

template <typename T1, typename T2>
constexpr bool operator!=(T1 const& lhs, indirect<T2> const& rhs) noexcept
{
    return !(lhs == rhs);
}

template <typename T1, typename T2>
constexpr bool operator<(indirect<T1> const& lhs, indirect<T2> const& rhs) noexcept
{
    return std::less<std::common_type_t<T1*, T2*>>()(get_pointer(lhs), get_pointer(rhs));
}

template <typename T1, typename T2>
constexpr bool operator>(indirect<T1> const& lhs, indirect<T2> const& rhs) noexcept
{
    return rhs < lhs;
}

template <typename T1, typename T2>
constexpr bool operator<=(indirect<T1> const& lhs, indirect<T2> const& rhs) noexcept
{
    return !(rhs < lhs);
}

template <typename T1, typename T2>
constexpr bool operator>=(indirect<T1> const& lhs, indirect<T2> const& rhs) noexcept
{
    return !(lhs < rhs);
}

template <typename T>
std::ostream& operator<<(std::ostream& s, indirect<T> const& v)
{
    return s << get_pointer(v);
}

template <typename T>
std::istream& operator>>(std::istream& s, indirect<T> const& v)
{
    return get_pointer(v) >> s;
}

namespace std
{

template <typename T>
struct hash<indirect<T>>
{
    constexpr std::size_t operator()(indirect<T> const& v) const noexcept
    {
        return hash<T*>()(get_pointer(v));
    }
};

} // namespace std

//===================
// optional_indirect
//===================

template <typename T>
class optional_indirect
{
public:
    using value_type = T;
    using const_type = optional_indirect<T const>;

private:
    T* target;

public:
    constexpr optional_indirect() noexcept :
        target()
    {
    }

    constexpr optional_indirect(nullopt_t) noexcept :
        target()
    {
    }

    constexpr optional_indirect(T& r) noexcept :
        target(&r)
    {
    }

    constexpr explicit optional_indirect(T* p) noexcept :
        target(p)
    {
    }

    template <typename U, typename = std::enable_if_t<std::is_convertible<U*, T*>::value>>
    constexpr optional_indirect(optional_indirect<U> const& v) noexcept :
        target(get_pointer(v))
    {
    }

    template <typename U, typename = std::enable_if_t<std::is_convertible<U*, T*>::value>>
    constexpr optional_indirect(indirect<U> const& v) noexcept :
        target(get_pointer(v))
    {
    }

    constexpr explicit operator bool() const noexcept
    {
        return target != nullptr;
    }

    constexpr T& operator*() const noexcept
    {
        return *target;
    }

    constexpr T* operator->() const noexcept
    {
        return target;
    }

    constexpr explicit operator T*() const noexcept
    {
        return target;
    }

    constexpr T& value() const
    {
        if (!target)
        {
            throw bad_optional_access();
        }

        return *target;
    }

    template <typename U, typename T_ = T, std::enable_if_t<std::is_copy_constructible<T_>::value, int> = 0>
    constexpr T value_or(U&& default_value) const noexcept
    {
        return target ? *target : static_cast<T>(std::forward<U>(default_value));
    }

    void swap(optional_indirect& other) noexcept
    {
        using std::swap;
        swap(target, other.target);
    }
};

template <typename T>
constexpr optional_indirect<T> make_optional_indirect(T& r) noexcept
{
    return r;
}

template <typename T, typename U>
constexpr optional_indirect<T> static_indirect_cast(optional_indirect<U> const& v) noexcept
{
    return optional_indirect<T>(static_cast<T*>(get_pointer(v)));
}

template <typename T, typename U>
constexpr optional_indirect<T> dynamic_indirect_cast(optional_indirect<U> const& v) noexcept
{
    return optional_indirect<T>(dynamic_cast<T*>(get_pointer(v)));
}

template <typename T, typename U>
constexpr optional_indirect<T> const_indirect_cast(optional_indirect<U> const& v) noexcept
{
    return optional_indirect<T>(const_cast<T*>(get_pointer(v)));
}

template <typename T>
constexpr T* get_pointer(optional_indirect<T> const& v) noexcept
{
    return static_cast<T*>(v);
}

template <typename T>
constexpr T* get_pointer(optional_indirect<T>& v) noexcept
{
    return static_cast<T*>(v);
}

template <typename T>
void swap(optional_indirect<T>& lhs, optional_indirect<T>& rhs) noexcept
{
    lhs.swap(rhs);
}

template <typename T1, typename T2>
constexpr bool operator==(optional_indirect<T1> const& lhs, optional_indirect<T2> const& rhs) noexcept
{
    return get_pointer(lhs) == get_pointer(rhs);
}

template <typename T1, typename T2>
constexpr bool operator==(optional_indirect<T1> const& lhs, indirect<T2> const& rhs) noexcept
{
    return get_pointer(lhs) == get_pointer(rhs);
}

template <typename T1, typename T2>
constexpr bool operator==(indirect<T1> const& lhs, optional_indirect<T2> const& rhs) noexcept
{
    return get_pointer(lhs) == get_pointer(rhs);
}

template <typename T1, typename T2, std::enable_if_t<std::is_convertible<T1*, T2*>::value || std::is_convertible<T2*, T1*>::value, int> = 0>
constexpr bool operator==(optional_indirect<T1> const& lhs, T2& rhs) noexcept
{
    return get_pointer(lhs) == &rhs;
}

template <typename T1, typename T2, std::enable_if_t<std::is_convertible<T1*, T2*>::value || std::is_convertible<T2*, T1*>::value, int> = 0>
constexpr bool operator==(T1& lhs, optional_indirect<T2> const& rhs) noexcept
{
    return &lhs == get_pointer(rhs);
}

template <typename T>
constexpr bool operator==(optional_indirect<T> const& lhs, nullopt_t) noexcept
{
    return !lhs;
}

template <typename T>
constexpr bool operator==(nullopt_t, optional_indirect<T> const& rhs) noexcept
{
    return !rhs;
}

template <typename T1, typename T2>
constexpr bool operator!=(optional_indirect<T1> const& lhs, optional_indirect<T2> const& rhs) noexcept
{
    return !(lhs == rhs);
}

template <typename T1, typename T2>
constexpr bool operator!=(optional_indirect<T1> const& lhs, indirect<T2> const& rhs) noexcept
{
    return !(lhs == rhs);
}

template <typename T1, typename T2>
constexpr bool operator!=(indirect<T1> const& lhs, optional_indirect<T2> const& rhs) noexcept
{
    return !(lhs == rhs);
}

template <typename T1, typename T2>
constexpr bool operator!=(optional_indirect<T1> const& lhs, T2 const& rhs) noexcept
{
    return !(lhs == rhs);
}

template <typename T1, typename T2>
constexpr bool operator!=(T1 const& lhs, optional_indirect<T2> const& rhs) noexcept
{
    return !(lhs == rhs);
}

template <typename T>
constexpr bool operator!=(optional_indirect<T> const& lhs, nullopt_t) noexcept
{
    return !(lhs == nullopt);
}

template <typename T>
constexpr bool operator!=(nullopt_t, optional_indirect<T> const& rhs) noexcept
{
    return !(nullopt == rhs);
}

template <typename T1, typename T2>
constexpr bool operator<(optional_indirect<T1> const& lhs, optional_indirect<T2> const& rhs) noexcept
{
    return std::less<std::common_type_t<T1*, T2*>>()(get_pointer(lhs), get_pointer(rhs));
}

template <typename T1, typename T2>
constexpr bool operator>(optional_indirect<T1> const& lhs, optional_indirect<T2> const& rhs) noexcept
{
    return rhs < lhs;
}

template <typename T1, typename T2>
constexpr bool operator<=(optional_indirect<T1> const& lhs, optional_indirect<T2> const& rhs) noexcept
{
    return !(rhs < lhs);
}

template <typename T1, typename T2>
constexpr bool operator>=(optional_indirect<T1> const& lhs, optional_indirect<T2> const& rhs) noexcept
{
    return !(lhs < rhs);
}

template <typename T>
std::ostream& operator<<(std::ostream& s, optional_indirect<T> const& v)
{
    return s << get_pointer(v);
}

template <typename T>
std::istream& operator>>(std::istream& s, optional_indirect<T> const& v)
{
    return get_pointer(v) >> s;
}

namespace std
{

template <typename T>
struct hash<optional_indirect<T>>
{
    constexpr std::size_t operator()(optional_indirect<T> const& v) const noexcept
    {
        return hash<T*>()(get_pointer(v));
    }
};

} // namespace std

#endif // INDIRECT_HPP
