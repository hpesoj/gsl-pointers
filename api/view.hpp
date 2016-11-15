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

#ifndef VIEW_HPP
#define VIEW_HPP

#include <functional>
#include <iosfwd>
#include <stdexcept>
#include <utility>

struct nullopt_t
{
    constexpr explicit nullopt_t(int) {}
};

constexpr nullopt_t nullopt{0};

class bad_optional_access : public std::logic_error
{
public:
    bad_optional_access() : std::logic_error(
        "Attempted to access the value of an uninitialized optional.")
    {
    }
};

template <typename>
class view;

template <typename>
class optional_view;

template <typename T>
constexpr T* get_pointer(view<T> const&) noexcept;

template <typename T>
constexpr T* get_pointer(optional_view<T> const&) noexcept;

template <typename T>
class view
{
public:
    using value_type = T;
    using const_type = view<T const>;

private:
    T* target;

    template <typename U>
    static constexpr U* throw_if_null(U* p)
    {
        if (!p)
        {
            throw std::invalid_argument("Cannot convert null pointer to view.");
        }

        return p;
    }

public:
    constexpr view(T& r) noexcept :
        target(&r)
    {
    }

    constexpr explicit view(T* p) :
        target(throw_if_null(p))
    {
    }

    template <typename U, typename = std::enable_if_t<std::is_convertible<U*, T*>::value>>
    constexpr view(view<U> const& v) noexcept :
        target(get_pointer(v))
    {
    }

    template <typename U, typename = std::enable_if_t<std::is_convertible<U*, T*>::value>>
    constexpr explicit view(optional_view<U> const& v) :
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

    constexpr operator T&() const noexcept
    {
        return *target;
    }

    constexpr explicit operator T*() const noexcept
    {
        return target;
    }

    void swap(view& other) noexcept
    {
        using std::swap;
        swap(target, other.target);
    }
};

template <typename T>
void swap(view<T>& lhs, view<T>& rhs)
{
    lhs.swap(rhs);
}

template <typename T1, typename T2>
constexpr bool operator==(view<T1> const& lhs, view<T2> const& rhs) noexcept
{
    return get_pointer(lhs) == get_pointer(rhs);
}

template <typename T1, typename T2>
constexpr bool operator==(view<T1> const& lhs, T2 const& rhs) noexcept
{
    return get_pointer(lhs) == &rhs;
}

template <typename T1, typename T2>
constexpr bool operator==(T1 const& lhs, view<T2> const& rhs) noexcept
{
    return &lhs == get_pointer(rhs);
}

template <typename T1, typename T2>
constexpr bool operator!=(view<T1> const& lhs, view<T2> const& rhs) noexcept
{
    return !(lhs == rhs);
}

template <typename T1, typename T2>
constexpr bool operator!=(view<T1> const& lhs, T2 const& rhs) noexcept
{
    return !(lhs == rhs);
}

template <typename T1, typename T2>
constexpr bool operator!=(T1 const& lhs, view<T2> const& rhs) noexcept
{
    return !(lhs == rhs);
}

template <typename T1, typename T2>
constexpr bool operator<(view<T1> const& lhs, view<T2> const& rhs) noexcept
{
    return std::less<std::common_type_t<T1*, T2*>>()(get_pointer(lhs), get_pointer(rhs));
}

template <typename T1, typename T2>
constexpr bool operator>(view<T1> const& lhs, view<T2> const& rhs) noexcept
{
    return rhs < lhs;
}

template <typename T1, typename T2>
constexpr bool operator<=(view<T1> const& lhs, view<T2> const& rhs) noexcept
{
    return !(rhs < lhs);
}

template <typename T1, typename T2>
constexpr bool operator>=(view<T1> const& lhs, view<T2> const& rhs) noexcept
{
    return !(lhs < rhs);
}

template <typename T>
class optional_view
{
public:
    using value_type = T;
    using const_type = optional_view<T const>;

private:
    T* target;

public:
    constexpr optional_view() noexcept :
        target()
    {
    }

    constexpr optional_view(nullopt_t) noexcept :
        target()
    {
    }

    constexpr optional_view(T& r) noexcept :
        target(&r)
    {
    }

    constexpr explicit optional_view(std::nullptr_t) noexcept :
        target(nullptr)
    {
    }

    constexpr explicit optional_view(T* p) noexcept :
        target(p)
    {
    }

    template <typename U, typename = std::enable_if_t<std::is_convertible<U*, T*>::value>>
    constexpr optional_view(optional_view<U> const& v) noexcept :
        target(get_pointer(v))
    {
    }

    template <typename U, typename = std::enable_if_t<std::is_convertible<U*, T*>::value>>
    constexpr optional_view(view<U> const& v) noexcept :
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

    constexpr explicit operator T&() const
    {
        return value();
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

    void swap(optional_view& other) noexcept
    {
        using std::swap;
        swap(target, other.target);
    }
};

template <typename T>
void swap(optional_view<T>& lhs, optional_view<T>& rhs) noexcept
{
    lhs.swap(rhs);
}

template <typename T1, typename T2>
constexpr bool operator==(optional_view<T1> const& lhs, optional_view<T2> const& rhs) noexcept
{
    return get_pointer(lhs) == get_pointer(rhs);
}

template <typename T1, typename T2>
constexpr bool operator==(optional_view<T1> const& lhs, view<T2> const& rhs) noexcept
{
    return get_pointer(lhs) == get_pointer(rhs);
}

template <typename T1, typename T2>
constexpr bool operator==(view<T1> const& lhs, optional_view<T2> const& rhs) noexcept
{
    return get_pointer(lhs) == get_pointer(rhs);
}

template <typename T1, typename T2>
constexpr bool operator==(optional_view<T1> const& lhs, T2 const& rhs) noexcept
{
    return get_pointer(lhs) == &rhs;
}

template <typename T1, typename T2>
constexpr bool operator==(T1 const& lhs, optional_view<T2> const& rhs) noexcept
{
    return &lhs == get_pointer(rhs);
}

template <typename T>
constexpr bool operator==(optional_view<T> const& lhs, nullopt_t) noexcept
{
    return !lhs;
}

template <typename T>
constexpr bool operator==(nullopt_t, optional_view<T> const& rhs) noexcept
{
    return !rhs;
}

template <typename T1, typename T2>
constexpr bool operator!=(optional_view<T1> const& lhs, optional_view<T2> const& rhs) noexcept
{
    return !(lhs == rhs);
}

template <typename T1, typename T2>
constexpr bool operator!=(optional_view<T1> const& lhs, view<T2> const& rhs) noexcept
{
    return !(lhs == rhs);
}

template <typename T1, typename T2>
constexpr bool operator!=(view<T1> const& lhs, optional_view<T2> const& rhs) noexcept
{
    return !(lhs == rhs);
}

template <typename T1, typename T2>
constexpr bool operator!=(optional_view<T1> const& lhs, T2 const& rhs) noexcept
{
    return !(lhs == rhs);
}

template <typename T1, typename T2>
constexpr bool operator!=(T1 const& lhs, optional_view<T2> const& rhs) noexcept
{
    return !(lhs == rhs);
}

template <typename T>
constexpr bool operator!=(optional_view<T> const& lhs, nullopt_t) noexcept
{
    return !(lhs == nullopt);
}

template <typename T>
constexpr bool operator!=(nullopt_t, optional_view<T> const& rhs) noexcept
{
    return !(nullopt == rhs);
}

template <typename T1, typename T2>
constexpr bool operator<(optional_view<T1> const& lhs, optional_view<T2> const& rhs) noexcept
{
    return std::less<std::common_type_t<T1*, T2*>>()(get_pointer(lhs), get_pointer(rhs));
}

template <typename T1, typename T2>
constexpr bool operator>(optional_view<T1> const& lhs, optional_view<T2> const& rhs) noexcept
{
    return rhs < lhs;
}

template <typename T1, typename T2>
constexpr bool operator<=(optional_view<T1> const& lhs, optional_view<T2> const& rhs) noexcept
{
    return !(rhs < lhs);
}

template <typename T1, typename T2>
constexpr bool operator>=(optional_view<T1> const& lhs, optional_view<T2> const& rhs) noexcept
{
    return !(lhs < rhs);
}

template <typename T>
constexpr view<T> make_view(T& r) noexcept
{
    return view<T>(r);
}

template <typename T>
constexpr optional_view<T> make_optional_view(T& r) noexcept
{
    return optional_view<T>(r);
}

template <typename T, typename U>
constexpr view<T> static_view_cast(view<U> const& v) noexcept
{
    return static_cast<T&>(*v);
}

template <typename T, typename U>
constexpr view<T> dynamic_view_cast(view<U> const& v)
{
    return dynamic_cast<T&>(*v);
}

template <typename T, typename U>
constexpr view<T> const_view_cast(view<U> const& v) noexcept
{
    return const_cast<T&>(*v);
}

template <typename T, typename U>
constexpr optional_view<T> static_view_cast(optional_view<U> const& v) noexcept
{
    return optional_view<T>(static_cast<T*>(get_pointer(v)));
}

template <typename T, typename U>
constexpr optional_view<T> dynamic_view_cast(optional_view<U> const& v) noexcept
{
    return optional_view<T>(dynamic_cast<T*>(get_pointer(v)));
}

template <typename T, typename U>
constexpr optional_view<T> const_view_cast(optional_view<U> const& v) noexcept
{
    return optional_view<T>(const_cast<T*>(get_pointer(v)));
}

template <typename T>
std::ostream& operator<<(std::ostream& s, view<T> const& v)
{
    return s << get_pointer(v);
}

template <typename T>
std::istream& operator>>(std::istream& s, view<T> const& v)
{
    return get_pointer(v) >> s;
}

template <typename T>
std::ostream& operator<<(std::ostream& s, optional_view<T> const& v)
{
    return s << get_pointer(v);
}

template <typename T>
std::istream& operator>>(std::istream& s, optional_view<T> const& v)
{
    return get_pointer(v) >> s;
}

template <typename T>
constexpr T* get_pointer(view<T> const& v) noexcept
{
    return static_cast<T*>(v);
}

template <typename T>
constexpr T* get_pointer(optional_view<T> const& v) noexcept
{
    return static_cast<T*>(v);
}

namespace std
{

template <typename T>
struct hash<view<T>>
{
    constexpr std::size_t operator()(view<T> const& v) const noexcept
    {
        return hash<T*>()(get_pointer(v));
    }
};

template <typename T>
struct hash<optional_view<T>>
{
    constexpr std::size_t operator()(optional_view<T> const& v) const noexcept
    {
        return hash<T*>()(get_pointer(v));
    }
};

} // namespace std

#endif // VIEW_HPP
