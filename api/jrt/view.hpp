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

public:
    constexpr view(element_type& r) noexcept : element(&r)
    {
    }

    view& operator=(element_type& r) noexcept
    {
        element = &r;
        return *this;
    }

    constexpr view(element_type&& r) noexcept = delete;
    view& operator=(element_type&& r) noexcept = delete;

    template <typename U, typename = std::enable_if_t<std::is_convertible<U&, element_type&>::value>>
    constexpr view(view<U> const& v) noexcept : element(v.get())
    {
    }

    template <typename U, typename = std::enable_if_t<std::is_convertible<U&, element_type&>::value>>
    view& operator=(view<U> const& v) noexcept
    {
        element = v.get();
        return *this;
    }

    template <typename U, typename = std::enable_if_t<std::is_convertible<U&, element_type&>::value>>
    constexpr view(view<U>&& v) noexcept :
        view(v)
    {
    }

    template <typename U, typename = std::enable_if_t<std::is_convertible<U&, element_type&>::value>>
    view& operator=(view<U>&& v) noexcept
    {
        return *this = v;
    }

    constexpr element_type* operator->() const noexcept
    {
        return element;
    }

    constexpr element_type& operator*() const noexcept
    {
        return *element;
    }

    constexpr bool has_value() const noexcept
    {
        return true;
    }

    // Exists to support std::propagate_const.
    constexpr explicit operator bool() const noexcept
    {
        return has_value();
    }

    constexpr element_type& value() const noexcept
    {
        return **this;
    }

    constexpr operator element_type() const noexcept = delete;

    template <typename U, typename = std::enable_if_t<std::is_convertible<element_type&, U&>::value>>
    constexpr operator U&() const noexcept
    {
        return value();
    }

    template <typename U, typename = std::enable_if_t<std::is_convertible<element_type*, U*>::value>>
    constexpr operator U*() const noexcept
    {
        return element;
    }

    // Exists to support std::propagate_const.
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
constexpr bool operator==(view<T1> const& lhs, T2* rhs) noexcept
{
    return lhs.get() == rhs;
}

template <typename T1, typename T2>
constexpr bool operator==(T1* lhs, view<T2> const& rhs) noexcept
{
    return lhs == rhs.get();
}

template <typename T1, typename T2>
constexpr bool operator!=(view<T1> const& lhs, view<T2> const& rhs) noexcept
{
    return !(lhs == rhs);
}

template <typename T1, typename T2>
constexpr bool operator!=(view<T1> const& lhs, T2 const& rhs) noexcept
{
    return lhs.get() != &rhs;
}

template <typename T1, typename T2>
constexpr bool operator!=(T1 const& lhs, view<T2> const& rhs) noexcept
{
    return &lhs != rhs.get();
}

template <typename T1, typename T2>
constexpr bool operator!=(view<T1> const& lhs, T2* rhs) noexcept
{
    return lhs.get() != rhs;
}

template <typename T1, typename T2>
constexpr bool operator!=(T1* lhs, view<T2> const& rhs) noexcept
{
    return lhs != rhs.get();
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

template <typename T>
void swap(view<T>& lhs, view<T>& rhs)
{
    lhs.swap(rhs);
}

//---------------
// optional_view
//===============

struct nullopt_t
{
};

constexpr nullopt_t nullopt;

class bad_optional_view_access : public std::logic_error
{
public:
    bad_optional_view_access() : std::logic_error(
        "Attempted to access the value of an uninitialized optional view.")
    {
    }
};

template <typename T>
class optional_view
{
public:
    using element_type = T;

private:
    element_type* element;

public:
    constexpr optional_view() noexcept :
        optional_view(nullptr)
    {
    }

    constexpr optional_view(std::nullptr_t) noexcept :
        element(nullptr)
    {
    }

    optional_view& operator=(std::nullptr_t) noexcept
    {
        element = nullptr;
        return *this;
    }

    constexpr optional_view(element_type* p) noexcept :
        element(p)
    {
    }

    optional_view& operator=(element_type* p) noexcept
    {
        element = p;
        return *this;
    }

    constexpr optional_view(nullopt_t) noexcept :
        optional_view(nullptr)
    {
    }

    optional_view& operator=(nullopt_t) noexcept
    {
        return *this = nullptr;
    }

    constexpr optional_view(element_type& r) noexcept :
        optional_view(&r)
    {
    }

    optional_view& operator=(element_type& r) noexcept
    {
        return *this = &r;
    }

    template <typename U, typename = std::enable_if_t<std::is_convertible<U&, element_type&>::value>>
    constexpr optional_view(optional_view<U> const& v) noexcept :
        element(v.get())
    {
    }

    template <typename U, typename = std::enable_if_t<std::is_convertible<U&, element_type&>::value>>
    optional_view& operator=(optional_view<U> const& v) noexcept
    {
        element = v.get();
        return *this;
    }

    template <typename U, typename = std::enable_if_t<std::is_convertible<U&, element_type&>::value>>
    constexpr optional_view(optional_view<U>&& v) noexcept :
        optional_view(v)
    {
    }

    template <typename U, typename = std::enable_if_t<std::is_convertible<U&, element_type&>::value>>
    optional_view& operator=(optional_view<U>&& v) noexcept
    {
        return *this = v;
    }

    template <typename U, typename = std::enable_if_t<std::is_convertible<U&, element_type&>::value>>
    constexpr optional_view(view<U> const& v) noexcept :
        element(v.get())
    {
    }

    template <typename U, typename = std::enable_if_t<std::is_convertible<U&, element_type&>::value>>
    optional_view& operator=(view<U> const& v) noexcept
    {
        element = v.get();
        return *this;
    }

    template <typename U, typename = std::enable_if_t<std::is_convertible<U&, element_type&>::value>>
    constexpr optional_view(view<U>&& v) noexcept :
        optional_view(v)
    {
    }

    template <typename U, typename = std::enable_if_t<std::is_convertible<U&, element_type&>::value>>
    optional_view& operator=(view<U>&& v) noexcept
    {
        return *this = v;
    }

    constexpr element_type* operator->() const noexcept
    {
        return element;
    }

    constexpr element_type& operator*() const noexcept
    {
        return *element;
    }

    constexpr bool has_value() const noexcept
    {
        return element != nullptr;
    }

    constexpr explicit operator bool() const noexcept
    {
        return has_value();
    }

    constexpr element_type& value() const
    {
        if (!has_value())
        {
            throw bad_optional_view_access();
        }

        return **this;
    }

    template <typename U, typename = std::enable_if_t<std::is_convertible<element_type*, U*>::value>>
    constexpr operator U*() const noexcept
    {
        return element;
    }

    template <typename U, typename = std::enable_if_t<std::is_copy_constructible<element_type>::value>>
    constexpr element_type value_or(U&& default_value) const noexcept
    {
        return has_value() ? **this : static_cast<element_type>(std::forward<U>(default_value));
    }

    void reset() noexcept
    {
        element = nullptr;
    }

    // Exists to support std::propagate_const.
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
constexpr bool operator==(optional_view<T1> const& lhs, T2* rhs) noexcept
{
    return lhs.get() == rhs;
}

template <typename T1, typename T2>
constexpr bool operator==(T1* lhs, optional_view<T2> const& rhs) noexcept
{
    return lhs == rhs.get();
}

template <typename T>
constexpr bool operator==(optional_view<T> const& lhs, std::nullptr_t) noexcept
{
    return !lhs;
}

template <typename T>
constexpr bool operator==(std::nullptr_t, optional_view<T> const& rhs) noexcept
{
    return rhs == nullptr;
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
    return !lhs;
}

template <typename T>
constexpr bool operator==(nullopt_t, optional_view<T> const& rhs) noexcept
{
    return rhs == nullopt;
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
constexpr bool operator!=(optional_view<T1> const& lhs, T2* rhs) noexcept
{
    return lhs.get() != rhs;
}

template <typename T1, typename T2>
constexpr bool operator!=(T1* lhs, optional_view<T2> const& rhs) noexcept
{
    return lhs != rhs.get();
}

template <typename T>
constexpr bool operator!=(optional_view<T> const& lhs, std::nullptr_t) noexcept
{
    return !(lhs == nullptr);
}

template <typename T>
constexpr bool operator!=(std::nullptr_t, optional_view<T> const& rhs) noexcept
{
    return !(nullptr == rhs);
}

template <typename T1, typename T2>
constexpr bool operator!=(optional_view<T1> const& lhs, T2 const& rhs) noexcept
{
    return lhs.get() != &rhs;
}

template <typename T1, typename T2>
constexpr bool operator!=(T1 const& lhs, optional_view<T2> const& rhs) noexcept
{
    return &lhs != rhs.get();
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

template <typename T>
void swap(optional_view<T>& lhs, optional_view<T>& rhs) noexcept
{
    lhs.swap(rhs);
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
constexpr optional_view<T> dynamic_view_cast(view<U> const& v) noexcept
{
    return make_optional_view(dynamic_cast<T*>(v.get()));
}

template <typename T, typename U>
constexpr view<T> const_view_cast(view<U> const& v) noexcept
{
    return const_cast<T&>(*v);
}

template <typename T, typename U>
constexpr view<T> reinterpret_view_cast(view<U> const& v) noexcept
{
    return reinterpret_cast<T&>(*v);
}

template <typename T, typename U>
constexpr optional_view<T> static_view_cast(optional_view<U> const& v) noexcept
{
    return static_cast<T*>(v.get());
}

template <typename T, typename U>
constexpr optional_view<T> dynamic_view_cast(optional_view<U> const& v) noexcept
{
    return dynamic_cast<T*>(v.get());
}

template <typename T, typename U>
constexpr optional_view<T> const_view_cast(optional_view<U> const& v) noexcept
{
    return const_cast<T*>(v.get());
}

template <typename T, typename U>
constexpr optional_view<T> reinterpret_view_cast(optional_view<U> const& v) noexcept
{
    return reinterpret_cast<T*>(v.get());
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
