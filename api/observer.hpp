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

#ifndef OBSERVER_HPP
#define OBSERVER_HPP

#include <functional>
#include <iosfwd>
#include <utility>

//==========
// observer
//==========

template <typename T>
class observer;

template <typename T>
constexpr T* get_pointer(observer<T> const&) noexcept;

template <typename T>
class observer
{
public:
    using element_type = T;
    using const_type = observer<std::add_const_t<T>>;

private:
    T* target;

public:
    constexpr observer(T& r) noexcept :
        target(&r)
    {
    }

    constexpr observer(T&&) noexcept = delete;

    template <typename U, typename = std::enable_if_t<std::is_convertible<U*, T*>::value>>
    constexpr observer(observer<U> const& i) noexcept :
        target(get_pointer(i))
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

    void swap(observer& other) noexcept
    {
        using std::swap;
        swap(target, other.target);
    }
};

template <typename T>
constexpr observer<T> make_observer(T& r) noexcept
{
    return r;
}

template <typename T>
observer<T> make_observer(T&&) = delete;

template <typename T>
constexpr T* get_pointer(observer<T> const& i) noexcept
{
    return static_cast<T*>(i);
}

template <typename T>
void swap(observer<T>& lhs, observer<T>& rhs)
{
    lhs.swap(rhs);
}

template <typename T1, typename T2>
constexpr bool operator==(observer<T1> const& lhs, observer<T2> const& rhs) noexcept
{
    return get_pointer(lhs) == get_pointer(rhs);
}

template <typename T1, typename T2, std::enable_if_t<std::is_convertible<T1*, T2*>::value || std::is_convertible<T2*, T1*>::value, int> = 0>
constexpr bool operator==(observer<T1> const& lhs, T2& rhs) noexcept
{
    return get_pointer(lhs) == &rhs;
}

template <typename T1, typename T2, std::enable_if_t<std::is_convertible<T1*, T2*>::value || std::is_convertible<T2*, T1*>::value, int> = 0>
constexpr bool operator==(T1& lhs, observer<T2> const& rhs) noexcept
{
    return &lhs == get_pointer(rhs);
}

template <typename T1, typename T2>
constexpr bool operator!=(observer<T1> const& lhs, observer<T2> const& rhs) noexcept
{
    return !(lhs == rhs);
}

template <typename T1, typename T2, std::enable_if_t<std::is_convertible<T1*, T2*>::value || std::is_convertible<T2*, T1*>::value, int> = 0>
constexpr bool operator!=(observer<T1> const& lhs, T2& rhs) noexcept
{
    return !(lhs == rhs);
}

template <typename T1, typename T2, std::enable_if_t<std::is_convertible<T1*, T2*>::value || std::is_convertible<T2*, T1*>::value, int> = 0>
constexpr bool operator!=(T1& lhs, observer<T2> const& rhs) noexcept
{
    return !(lhs == rhs);
}

template <typename T1, typename T2>
constexpr bool operator<(observer<T1> const& lhs, observer<T2> const& rhs) noexcept
{
    return std::less<std::common_type_t<T1*, T2*>>()(get_pointer(lhs), get_pointer(rhs));
}

template <typename T1, typename T2, std::enable_if_t<std::is_convertible<T1*, T2*>::value || std::is_convertible<T2*, T1*>::value, int> = 0>
constexpr bool operator<(observer<T1> const& lhs, T2& rhs) noexcept
{
    return std::less<std::common_type_t<T1*, T2*>>()(get_pointer(lhs), &rhs);
}

template <typename T1, typename T2, std::enable_if_t<std::is_convertible<T1*, T2*>::value || std::is_convertible<T2*, T1*>::value, int> = 0>
constexpr bool operator<(T1& lhs, observer<T2> const& rhs) noexcept
{
    return std::less<std::common_type_t<T1*, T2*>>()(&lhs, get_pointer(rhs));
}

template <typename T1, typename T2>
constexpr bool operator>(observer<T1> const& lhs, observer<T2> const& rhs) noexcept
{
    return rhs < lhs;
}

template <typename T1, typename T2, std::enable_if_t<std::is_convertible<T1*, T2*>::value || std::is_convertible<T2*, T1*>::value, int> = 0>
constexpr bool operator>(observer<T1> const& lhs, T2& rhs) noexcept
{
    return rhs < lhs;
}

template <typename T1, typename T2, std::enable_if_t<std::is_convertible<T1*, T2*>::value || std::is_convertible<T2*, T1*>::value, int> = 0>
constexpr bool operator>(T1& lhs, observer<T2> const& rhs) noexcept
{
    return rhs < lhs;
}

template <typename T1, typename T2>
constexpr bool operator<=(observer<T1> const& lhs, observer<T2> const& rhs) noexcept
{
    return !(rhs < lhs);
}

template <typename T1, typename T2, std::enable_if_t<std::is_convertible<T1*, T2*>::value || std::is_convertible<T2*, T1*>::value, int> = 0>
constexpr bool operator<=(observer<T1> const& lhs, T2& rhs) noexcept
{
    return !(rhs < lhs);
}

template <typename T1, typename T2, std::enable_if_t<std::is_convertible<T1*, T2*>::value || std::is_convertible<T2*, T1*>::value, int> = 0>
constexpr bool operator<=(T1& lhs, observer<T2> const& rhs) noexcept
{
    return !(rhs < lhs);
}

template <typename T1, typename T2>
constexpr bool operator>=(observer<T1> const& lhs, observer<T2> const& rhs) noexcept
{
    return !(lhs < rhs);
}

template <typename T1, typename T2, std::enable_if_t<std::is_convertible<T1*, T2*>::value || std::is_convertible<T2*, T1*>::value, int> = 0>
constexpr bool operator>=(observer<T1> const& lhs, T2& rhs) noexcept
{
    return !(lhs < rhs);
}

template <typename T1, typename T2, std::enable_if_t<std::is_convertible<T1*, T2*>::value || std::is_convertible<T2*, T1*>::value, int> = 0>
constexpr bool operator>=(T1& lhs, observer<T2> const& rhs) noexcept
{
    return !(lhs < rhs);
}

template <typename T>
std::ostream& operator<<(std::ostream& s, observer<T> const& i)
{
    return s << get_pointer(i);
}

namespace std
{

template <typename T>
struct hash<observer<T>>
{
    constexpr std::size_t operator()(observer<T> const& i) const noexcept
    {
        return hash<T*>()(get_pointer(i));
    }
};

} // namespace std

//==============
// observer_ptr
//==============

template <typename T>
class observer_ptr;

template <typename T>
constexpr T* get_pointer(observer_ptr<T> const&) noexcept;

template <typename T>
class observer_ptr
{
public:
    using element_type = T;
    using const_type = observer_ptr<T const>;

private:
    T* target;

public:
    constexpr observer_ptr() noexcept :
        target()
    {
    }

    constexpr observer_ptr(T& r) noexcept :
        target(&r)
    {
    }

    constexpr observer_ptr(T&&) noexcept = delete;

    constexpr observer_ptr(observer<T> const& i) noexcept :
        target(get_pointer(i))
    {
    }

    template <typename U, typename = std::enable_if_t<std::is_convertible<U*, T*>::value>>
    constexpr observer_ptr(observer<U> const& i) noexcept :
        target(get_pointer(i))
    {
    }

    template <typename U, typename = std::enable_if_t<std::is_convertible<U*, T*>::value>>
    constexpr observer_ptr(observer_ptr<U> const& i) noexcept :
        target(get_pointer(i))
    {
    }

    constexpr observer_ptr(std::nullptr_t) noexcept :
        target(nullptr)
    {
    }

    constexpr explicit observer_ptr(T* p) noexcept :
        target(p)
    {
    }

    constexpr explicit operator bool() const noexcept
    {
        return target != nullptr;
    }

    constexpr T& operator*() const
    {
        return *target;
    }

    constexpr T* operator->() const
    {
        return target;
    }

    constexpr explicit operator T*() const noexcept
    {
        return target;
    }

    void swap(observer_ptr& other) noexcept
    {
        using std::swap;
        swap(target, other.target);
    }
};

template <typename T>
constexpr T* get_pointer(observer_ptr<T> const& i) noexcept
{
    return static_cast<T*>(i);
}

template <typename T>
void swap(observer_ptr<T>& lhs, observer_ptr<T>& rhs) noexcept
{
    lhs.swap(rhs);
}

template <typename T1, typename T2>
constexpr bool operator==(observer_ptr<T1> const& lhs, observer_ptr<T2> const& rhs) noexcept
{
    return get_pointer(lhs) == get_pointer(rhs);
}

template <typename T1, typename T2>
constexpr bool operator==(observer_ptr<T1> const& lhs, observer<T2> const& rhs) noexcept
{
    return get_pointer(lhs) == get_pointer(rhs);
}

template <typename T1, typename T2>
constexpr bool operator==(observer<T1> const& lhs, observer_ptr<T2> const& rhs) noexcept
{
    return get_pointer(lhs) == get_pointer(rhs);
}

template <typename T1, typename T2, std::enable_if_t<std::is_convertible<T1*, T2*>::value || std::is_convertible<T2*, T1*>::value, int> = 0>
constexpr bool operator==(observer_ptr<T1> const& lhs, T2& rhs) noexcept
{
    return get_pointer(lhs) == &rhs;
}

template <typename T1, typename T2, std::enable_if_t<std::is_convertible<T1*, T2*>::value || std::is_convertible<T2*, T1*>::value, int> = 0>
constexpr bool operator==(T1& lhs, observer_ptr<T2> const& rhs) noexcept
{
    return &lhs == get_pointer(rhs);
}

template <typename T>
constexpr bool operator==(observer_ptr<T> const& lhs, std::nullptr_t) noexcept
{
    return !lhs;
}

template <typename T>
constexpr bool operator==(std::nullptr_t, observer_ptr<T> const& rhs) noexcept
{
    return !rhs;
}

template <typename T1, typename T2>
constexpr bool operator!=(observer_ptr<T1> const& lhs, observer_ptr<T2> const& rhs) noexcept
{
    return !(lhs == rhs);
}

template <typename T1, typename T2>
constexpr bool operator!=(observer_ptr<T1> const& lhs, observer<T2> const& rhs) noexcept
{
    return !(lhs == rhs);
}

template <typename T1, typename T2>
constexpr bool operator!=(observer<T1> const& lhs, observer_ptr<T2> const& rhs) noexcept
{
    return !(lhs == rhs);
}

template <typename T1, typename T2, std::enable_if_t<std::is_convertible<T1*, T2*>::value || std::is_convertible<T2*, T1*>::value, int> = 0>
constexpr bool operator!=(observer_ptr<T1> const& lhs, T2& rhs) noexcept
{
    return !(lhs == rhs);
}

template <typename T1, typename T2, std::enable_if_t<std::is_convertible<T1*, T2*>::value || std::is_convertible<T2*, T1*>::value, int> = 0>
constexpr bool operator!=(T1& lhs, observer_ptr<T2> const& rhs) noexcept
{
    return !(lhs == rhs);
}

template <typename T>
constexpr bool operator!=(observer_ptr<T> const& lhs, std::nullptr_t) noexcept
{
    return !(lhs == nullptr);
}

template <typename T>
constexpr bool operator!=(std::nullptr_t, observer_ptr<T> const& rhs) noexcept
{
    return !(nullptr == rhs);
}

template <typename T1, typename T2>
constexpr bool operator<(observer_ptr<T1> const& lhs, observer_ptr<T2> const& rhs) noexcept
{
    return std::less<std::common_type_t<T1*, T2*>>()(get_pointer(lhs), get_pointer(rhs));
}

template <typename T1, typename T2>
constexpr bool operator<(observer_ptr<T1> const& lhs, observer<T2> const& rhs) noexcept
{
    return std::less<std::common_type_t<T1*, T2*>>()(get_pointer(lhs), get_pointer(rhs));
}

template <typename T1, typename T2>
constexpr bool operator<(observer<T1> const& lhs, observer_ptr<T2> const& rhs) noexcept
{
    return std::less<std::common_type_t<T1*, T2*>>()(get_pointer(lhs), get_pointer(rhs));
}

template <typename T1, typename T2, std::enable_if_t<std::is_convertible<T1*, T2*>::value || std::is_convertible<T2*, T1*>::value, int> = 0>
constexpr bool operator<(observer_ptr<T1> const& lhs, T2& rhs) noexcept
{
    return std::less<std::common_type_t<T1*, T2*>>()(get_pointer(lhs), &rhs);
}

template <typename T1, typename T2, std::enable_if_t<std::is_convertible<T1*, T2*>::value || std::is_convertible<T2*, T1*>::value, int> = 0>
constexpr bool operator<(T1& lhs, observer_ptr<T2> const& rhs) noexcept
{
    return std::less<std::common_type_t<T1*, T2*>>()(&lhs, get_pointer(rhs));
}

template <typename T>
constexpr bool operator<(observer_ptr<T> const& lhs, std::nullptr_t) noexcept
{
    return std::less<T*>()(get_pointer(lhs), nullptr);
}

template <typename T>
constexpr bool operator<(std::nullptr_t, observer_ptr<T> const& rhs) noexcept
{
    return std::less<T*>()(nullptr, get_pointer(rhs));
}

template <typename T1, typename T2>
constexpr bool operator>(observer_ptr<T1> const& lhs, observer_ptr<T2> const& rhs) noexcept
{
    return rhs < lhs;
}

template <typename T1, typename T2>
constexpr bool operator>(observer_ptr<T1> const& lhs, observer<T2> const& rhs) noexcept
{
    return rhs < lhs;
}

template <typename T1, typename T2>
constexpr bool operator>(observer<T1> const& lhs, observer_ptr<T2> const& rhs) noexcept
{
    return rhs < lhs;
}

template <typename T1, typename T2, std::enable_if_t<std::is_convertible<T1*, T2*>::value || std::is_convertible<T2*, T1*>::value, int> = 0>
constexpr bool operator>(observer_ptr<T1> const& lhs, T2& rhs) noexcept
{
    return rhs < lhs;
}

template <typename T1, typename T2, std::enable_if_t<std::is_convertible<T1*, T2*>::value || std::is_convertible<T2*, T1*>::value, int> = 0>
constexpr bool operator>(T1& lhs, observer_ptr<T2> const& rhs) noexcept
{
    return rhs < lhs;
}

template <typename T>
constexpr bool operator>(observer_ptr<T> const& lhs, std::nullptr_t) noexcept
{
    return nullptr < lhs;
}

template <typename T>
constexpr bool operator>(std::nullptr_t, observer_ptr<T> const& rhs) noexcept
{
    return rhs < nullptr;
}

template <typename T1, typename T2>
constexpr bool operator<=(observer_ptr<T1> const& lhs, observer_ptr<T2> const& rhs) noexcept
{
    return !(rhs < lhs);
}

template <typename T1, typename T2>
constexpr bool operator<=(observer_ptr<T1> const& lhs, observer<T2> const& rhs) noexcept
{
    return !(rhs < lhs);
}

template <typename T1, typename T2>
constexpr bool operator<=(observer<T1> const& lhs, observer_ptr<T2> const& rhs) noexcept
{
    return !(rhs < lhs);
}

template <typename T1, typename T2, std::enable_if_t<std::is_convertible<T1*, T2*>::value || std::is_convertible<T2*, T1*>::value, int> = 0>
constexpr bool operator<=(observer_ptr<T1> const& lhs, T2& rhs) noexcept
{
    return !(rhs < lhs);
}

template <typename T1, typename T2, std::enable_if_t<std::is_convertible<T1*, T2*>::value || std::is_convertible<T2*, T1*>::value, int> = 0>
constexpr bool operator<=(T1& lhs, observer_ptr<T2> const& rhs) noexcept
{
    return !(rhs < lhs);
}

template <typename T>
constexpr bool operator<=(observer_ptr<T> const& lhs, std::nullptr_t) noexcept
{
    return !(nullptr < lhs);
}

template <typename T>
constexpr bool operator<=(std::nullptr_t, observer_ptr<T> const& rhs) noexcept
{
    return !(rhs < nullptr);
}

template <typename T1, typename T2>
constexpr bool operator>=(observer_ptr<T1> const& lhs, observer_ptr<T2> const& rhs) noexcept
{
    return !(lhs < rhs);
}

template <typename T1, typename T2>
constexpr bool operator>=(observer_ptr<T1> const& lhs, observer<T2> const& rhs) noexcept
{
    return !(lhs < rhs);
}

template <typename T1, typename T2>
constexpr bool operator>=(observer<T1> const& lhs, observer_ptr<T2> const& rhs) noexcept
{
    return !(lhs < rhs);
}

template <typename T1, typename T2, std::enable_if_t<std::is_convertible<T1*, T2*>::value || std::is_convertible<T2*, T1*>::value, int> = 0>
constexpr bool operator>=(observer_ptr<T1> const& lhs, T2& rhs) noexcept
{
    return !(lhs < rhs);
}

template <typename T1, typename T2, std::enable_if_t<std::is_convertible<T1*, T2*>::value || std::is_convertible<T2*, T1*>::value, int> = 0>
constexpr bool operator>=(T1& lhs, observer_ptr<T2> const& rhs) noexcept
{
    return !(lhs < rhs);
}

template <typename T>
constexpr bool operator>=(observer_ptr<T> const& lhs, std::nullptr_t) noexcept
{
    return !(lhs < nullptr);
}

template <typename T>
constexpr bool operator>=(std::nullptr_t, observer_ptr<T> const& rhs) noexcept
{
    return !(nullptr < rhs);
}

template <typename T>
std::ostream& operator<<(std::ostream& s, observer_ptr<T> const& i)
{
    return s << get_pointer(i);
}

namespace std
{

template <typename T>
struct hash<observer_ptr<T>>
{
    constexpr std::size_t operator()(observer_ptr<T> const& i) const noexcept
    {
        return hash<T*>()(get_pointer(i));
    }
};

} // namespace std

#endif // OBSERVER_HPP
