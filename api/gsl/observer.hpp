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

#ifndef GSL_OBSERVER_HPP
#define GSL_OBSERVER_HPP

#include <functional>
#include <type_traits>

namespace gsl
{

template <typename T>
class observer
{
public:
    using element_type = T;

    constexpr explicit observer(T& t) noexcept :
        ptr(&t)
    {
    }

    constexpr explicit observer(T&&) noexcept = delete;

    template <typename U, typename = std::enable_if_t<std::is_convertible<U*, T*>::value>>
    constexpr observer(observer<U> const& o) noexcept :
        ptr(static_cast<U*>(o))
    {
    }

    constexpr T& operator*() const noexcept
    {
        return *ptr;
    }

    constexpr T* operator->() const noexcept
    {
        return ptr;
    }

    constexpr explicit operator T*() const noexcept
    {
        return ptr;
    }

    void swap(observer& other) noexcept
    {
        using std::swap;
        swap(ptr, other.ptr);
    }

private:
    T* ptr;
};

template <typename T>
constexpr observer<T> make_observer(T& t) noexcept
{
    return observer<T>(t);
}

template <typename T>
observer<T> make_observer(T&&) = delete;

template <typename T>
void swap(observer<T>& lhs, observer<T>& rhs)
{
    lhs.swap(rhs);
}

template <typename T1, typename T2>
constexpr bool operator==(observer<T1> const& lhs, observer<T2> const& rhs) noexcept
{
    return static_cast<T1*>(lhs) == static_cast<T2*>(rhs);
}

template <typename T1, typename T2>
constexpr bool operator!=(observer<T1> const& lhs, observer<T2> const& rhs) noexcept
{
    return !(lhs == rhs);
}

} // namespace gsl

namespace std
{

template <typename T>
struct less<gsl::observer<T>>
{
    constexpr bool operator()(gsl::observer<T> const& lhs, gsl::observer<T> const& rhs) const noexcept
    {
        return less<T*>()(static_cast<T*>(lhs), static_cast<T*>(rhs));
    }
};

template <typename T>
struct hash<gsl::observer<T>>
{
    constexpr std::size_t operator()(gsl::observer<T> const& o) const noexcept
    {
        return hash<T*>()(static_cast<T*>(o));
    }
};

} // namespace std

#endif // GSL_OBSERVER_HPP
