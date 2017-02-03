/* * Copyright (c) 2017 Joseph Thomson
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

#ifndef OPTIONAL_REF_HPP
#define OPTIONAL_REF_HPP

#include <type_traits>

struct nullref_t
{
    constexpr explicit nullref_t(int) noexcept {}
};

constexpr nullref_t const nullref{ 0 };

template <typename T>
class optional_ref
{
public:
    using value_type = T;

private:
    T* target;

public:
    constexpr optional_ref() noexcept :
        target()
    {
    }

    constexpr optional_ref(nullref_t) noexcept :
        target()
    {
    }

    constexpr optional_ref(T& r) noexcept :
        target(&r)
    {
    }
    optional_ref& operator=(optional_ref const&) = delete;

    template <typename U, typename = std::enable_if_t<std::is_convertible<U*, T*>::value>>
    constexpr optional_ref(optional_ref<U> const& o) noexcept :
        target(static_cast<U*>(o))
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

    constexpr bool has_value() const noexcept
    {
        return target != nullptr;
    }

    constexpr explicit operator bool() const noexcept
    {
        return has_value();
    }

    constexpr explicit operator T*() const noexcept
    {
        return target;
    }
};

template <typename T>
optional_ref<T> make_optional_ref(T& r)
{
    return r;
}

#endif // OPTIONAL_REF_HPP
