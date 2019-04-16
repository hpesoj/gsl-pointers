/*
 * Copyright (c) 2016 - 2019 Joseph Thomson
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#ifndef GSL_RETAINED_HPP
#define GSL_RETAINED_HPP

#include <cstddef>
#include <functional>
#include <memory>
#include <type_traits>
#include <utility>

namespace gsl
{
  template <typename T>
  class retained
  {
    template <typename>
    friend class retained;

  public:
    using element_type = T;

    constexpr explicit retained(T& t) noexcept
    : m_ptr(std::addressof(t))
    {
    }

    constexpr explicit retained(T&&) noexcept = delete;

    constexpr explicit retained(retained const&) noexcept = delete;
    constexpr explicit retained(retained&&) noexcept = default;

    template <typename U, typename = std::enable_if_t<std::is_convertible_v<U*, T*>>>
    constexpr retained(retained<U>&& other) noexcept
    : m_ptr(other.m_ptr)
    {
    }

    constexpr retained& operator=(retained const&) noexcept = delete;
    constexpr retained& operator=(retained&&) noexcept = default;

    template <typename U, typename = std::enable_if_t<std::is_convertible_v<U*, T*>>>
    retained& operator=(retained<U>&& other) noexcept
    {
      m_ptr = other.m_ptr;
      return *this;
    }

    constexpr T& operator*() noexcept
    {
      return *m_ptr;
    }

    constexpr T const& operator*() const noexcept
    {
      return *m_ptr;
    }

    constexpr T* operator->() noexcept
    {
      return m_ptr;
    }

    constexpr operator T const*() const noexcept
    {
      return m_ptr;
    }

    void swap(retained& other) noexcept
    {
      using std::swap;
      swap(m_ptr, other.m_ptr);
    }

  private:
    T* m_ptr;
  };

  template <typename T>
  constexpr retained<T> make_retained(T& t) noexcept
  {
    return retained<T>(t);
  }

  template <typename T>
  retained<T> make_retained(T&&) = delete;

  template <typename T>
  void swap(retained<T>& lhs, retained<T>& rhs) noexcept
  {
    lhs.swap(rhs);
  }

  template <typename T1, typename T2>
  constexpr bool operator==(retained<T1> const& lhs, retained<T2> const& rhs) noexcept
  {
    return &*lhs == &*rhs;
  }

  template <typename T1, typename T2>
  constexpr bool operator!=(retained<T1> const& lhs, retained<T2> const& rhs) noexcept
  {
    return !(lhs == rhs);
  }

} // namespace gsl

namespace std
{
  template <typename T>
  struct less<gsl::retained<T>>
  {
    constexpr bool operator()(gsl::retained<T> const& lhs, gsl::retained<T> const& rhs)
        const noexcept
    {
      return less<T const*>()(&*lhs, &*rhs);
    }
  };

  template <typename T>
  struct hash<gsl::retained<T>>
  {
    constexpr std::size_t operator()(gsl::retained<T> const& r) const noexcept
    {
      return hash<T const*>()(&*r);
    }
  };
} // namespace std

#endif // GSL_RETAINED_HPP
