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

#ifndef JRT_VIEW_HPP
#define JRT_VIEW_HPP

#include <functional>
#include <iosfwd>
#include <stdexcept>
#include <utility>

namespace jrt
{

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
class optional_view;

//------
// view
//======

template <typename T>
class view
{
public:
    using element_type = T;

private:
    element_type* element;

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
    constexpr view(element_type& r) noexcept :
        element(&r)
    {
    }

    constexpr explicit view(element_type* p) :
        element(throw_if_null(p))
    {
    }

    template <typename U, typename = std::enable_if_t<std::is_convertible<U*, element_type*>::value>>
    constexpr view(view<U> const& v) noexcept :
        element(v.get())
    {
    }

    template <typename U, typename = std::enable_if_t<std::is_convertible<U*, element_type*>::value>>
    constexpr explicit view(optional_view<U> const& v) :
        element(throw_if_null(v.get()))
    {
    }

    constexpr explicit operator bool() const noexcept
    {
        return true;
    }

    constexpr element_type& operator*() const noexcept
    {
        return *element;
    }

    constexpr element_type* operator->() const noexcept
    {
        return element;
    }

    constexpr operator element_type&() const noexcept
    {
        return value();
    }

    constexpr explicit operator element_type*() const noexcept
    {
        return element;
    }

    constexpr element_type& value() const noexcept
    {
        return *element;
    }

    constexpr element_type* get() const noexcept
    {
        return element;
    }

    void swap(view& other) noexcept
    {
        using std::swap;
        swap(element, other.element);
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
    return lhs.get() == rhs.get();
}

template <typename T1, typename T2>
constexpr bool operator==(view<T1> const& lhs, T2 const& rhs) noexcept
{
    return lhs.get() == &rhs;
}

template <typename T1, typename T2>
constexpr bool operator==(T1 const& lhs, view<T2> const& rhs) noexcept
{
    return &lhs == rhs.get();
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
    return std::less<std::common_type_t<T1*, T2*>>()(lhs.get(), rhs.get());
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

//---------------
// optional_view
//===============

template <typename T>
class optional_view
{
public:
    using element_type = T;

private:
    element_type* element;

public:
    constexpr optional_view() noexcept :
        element()
    {
    }

    constexpr optional_view(nullopt_t) noexcept :
        element(nullptr)
    {
    }

    constexpr optional_view(element_type& r) noexcept :
        element(&r)
    {
    }

    constexpr explicit optional_view(std::nullptr_t) noexcept :
        element()
    {
    }

    constexpr explicit optional_view(element_type* p) noexcept :
        element(p)
    {
    }

    template <typename U, typename = std::enable_if_t<std::is_convertible<U*, element_type*>::value>>
    constexpr optional_view(optional_view<U> const& v) noexcept :
        element(v.get())
    {
    }

    template <typename U, typename = std::enable_if_t<std::is_convertible<U*, element_type*>::value>>
    constexpr optional_view(view<U> const& v) noexcept :
        element(v.get())
    {
    }

    constexpr explicit operator bool() const noexcept
    {
        return element != nullptr;
    }

    constexpr element_type& operator*() const noexcept
    {
        return *element;
    }

    constexpr element_type* operator->() const noexcept
    {
        return element;
    }

    constexpr explicit operator element_type&() const
    {
        return value();
    }

    constexpr explicit operator element_type*() const noexcept
    {
        return element;
    }

    constexpr element_type& value() const
    {
        if (!element)
        {
            throw bad_optional_access();
        }

        return *element;
    }

    template <typename U, typename E = element_type, std::enable_if_t<std::is_copy_constructible<E>::value, int> = 0>
    constexpr element_type value_or(U&& default_value) const noexcept
    {
        return element ? *element : static_cast<element_type>(std::forward<U>(default_value));
    }

    constexpr element_type* get() const noexcept
    {
        return element;
    }

    void swap(optional_view& other) noexcept
    {
        using std::swap;
        swap(element, other.element);
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
    return lhs.get() == rhs.get();
}

template <typename T1, typename T2>
constexpr bool operator==(optional_view<T1> const& lhs, view<T2> const& rhs) noexcept
{
    return lhs.get() == rhs.get();
}

template <typename T1, typename T2>
constexpr bool operator==(view<T1> const& lhs, optional_view<T2> const& rhs) noexcept
{
    return lhs.get() == rhs.get();
}

template <typename T1, typename T2>
constexpr bool operator==(optional_view<T1> const& lhs, T2 const& rhs) noexcept
{
    return lhs.get() == &rhs;
}

template <typename T1, typename T2>
constexpr bool operator==(T1 const& lhs, optional_view<T2> const& rhs) noexcept
{
    return &lhs == rhs.get();
}

template <typename T>
constexpr bool operator==(optional_view<T> const& lhs, nullopt_t) noexcept
{
    return lhs.get() == nullptr;
}

template <typename T>
constexpr bool operator==(nullopt_t, optional_view<T> const& rhs) noexcept
{
    return nullptr == rhs.get();
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
    return std::less<std::common_type_t<T1*, T2*>>()(lhs.get(), rhs.get());
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

//----------------------
// non-member functions
//======================

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

template <typename T>
constexpr optional_view<T> make_optional_view(T* p) noexcept
{
    return optional_view<T>(p);
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
    return optional_view<T>(static_cast<T*>(v.get()));
}

template <typename T, typename U>
constexpr optional_view<T> dynamic_view_cast(optional_view<U> const& v) noexcept
{
    return optional_view<T>(dynamic_cast<T*>(v.get()));
}

template <typename T, typename U>
constexpr optional_view<T> const_view_cast(optional_view<U> const& v) noexcept
{
    return optional_view<T>(const_cast<T*>(v.get()));
}

//------------------
// stream operators
//==================

template <typename T>
std::ostream& operator<<(std::ostream& s, view<T> const& v)
{
    return s << v.get();
}

template <typename T>
std::istream& operator>>(std::istream& s, view<T> const& v)
{
    return v.get() >> s;
}

template <typename T>
std::ostream& operator<<(std::ostream& s, optional_view<T> const& v)
{
    return s << v.get();
}

template <typename T>
std::istream& operator>>(std::istream& s, optional_view<T> const& v)
{
    return v.get() >> s;
}

} // namespace jrt

//---------------------------
// std::hash specializations
//===========================

namespace std
{

template <typename T>
struct hash<jrt::view<T>>
{
    constexpr std::size_t operator()(jrt::view<T> const& v) const noexcept
    {
        return hash<T*>()(v.get());
    }
};

template <typename T>
struct hash<jrt::optional_view<T>>
{
    constexpr std::size_t operator()(jrt::optional_view<T> const& v) const noexcept
    {
        return hash<T*>()(v.get());
    }
};

} // namespace std

#endif // JRT_VIEW_HPP
