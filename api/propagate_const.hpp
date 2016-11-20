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

#ifndef PROPAGATE_CONST_HPP
#define PROPAGATE_CONST_HPP

#include <utility.hpp>

#include <functional>
#include <type_traits>
#include <utility>

template <typename T>
class propagate_const;

namespace detail {

    template <typename T>
    struct propagate_const_traits {
        using const_type = typename T::const_type;
        using const_pointer_type = decltype(std::declval<const_type&>().operator->());
        using pointer_type = decltype(std::declval<T&>().operator->());
        using const_reference_type = decltype(std::declval<const_type&>().operator*());
        using reference_type = decltype(std::declval<T&>().operator*());
    };
    template <typename T>
    struct propagate_const_traits<T*> {
        using const_type = T const*;
        using const_pointer_type = T const*;
        using pointer_type = T*;
        using const_reference_type = T const&;
        using reference_type = T&;
    };

    template <typename T>
    struct is_propagate_const : std::false_type {};
    template <typename T>
    struct is_propagate_const<propagate_const<T>> : std::true_type {};

    template <typename T>
    constexpr void swap_(T& lhs, T& rhs) noexcept(noexcept(swap(lhs, rhs))) {
        using std::swap;
        swap(lhs, rhs);
    }

} // namespace detail

template <typename T>
class propagate_const {
    template <typename U>
    friend class propagate_const;

private:
    T t;

public:
    using const_type = typename detail::propagate_const_traits<T>::const_type;
    using const_pointer_type = typename detail::propagate_const_traits<T>::const_pointer_type;
    using pointer_type = typename detail::propagate_const_traits<T>::pointer_type;
    using const_reference_type = typename detail::propagate_const_traits<T>::const_reference_type;
    using reference_type = typename detail::propagate_const_traits<T>::reference_type;

    constexpr propagate_const() noexcept(std::is_nothrow_default_constructible<T>::value) = default;

    constexpr propagate_const(propagate_const const&) noexcept(std::is_nothrow_copy_constructible<T>::value) = delete;
    /*constexpr*/ propagate_const& operator=(propagate_const const&) noexcept(std::is_nothrow_copy_assignable<T>::value) = delete;

    constexpr propagate_const(propagate_const&&) noexcept(std::is_nothrow_move_constructible<T>::value) = default;
    /*constexpr*/ propagate_const& operator=(propagate_const&&) noexcept(std::is_nothrow_move_assignable<T>::value) = default;

    template <typename U, std::enable_if_t<
        std::is_constructible<T, U&>::value &&
        std::is_convertible<U&, T>::value, int> = 0>
    constexpr propagate_const(propagate_const<U>& p) noexcept(std::is_nothrow_constructible<T, U>::value) :
        t(p.t) {
    }
    template <typename U, std::enable_if_t<
        std::is_constructible<T, U&>::value &&
        !std::is_convertible<U&, T>::value, int> = 0>
    constexpr explicit propagate_const(propagate_const<U>& p) noexcept(std::is_nothrow_constructible<T, U>::value) :
        t(p.t) {
    }
    template <typename U, std::enable_if_t<
        std::is_convertible<U&, T>::value, int> = 0>
    /*constexpr*/ propagate_const& operator=(propagate_const<U>& p) noexcept(std::is_nothrow_assignable<T, U>::value) {
        t = p.t;
        return *this;
    }

    template <typename U, std::enable_if_t<
        std::is_constructible<T, U&&>::value &&
        std::is_convertible<U&&, T>::value, int> = 0>
    /*constexpr*/ propagate_const(propagate_const<U>&& p) noexcept(std::is_nothrow_constructible<T, U&&>::value) :
        t(std::move(p.t)) {
    }
    template <typename U, std::enable_if_t<
        std::is_constructible<T, U&&>::value &&
        !std::is_convertible<U&&, T>::value, int> = 0>
    /*constexpr*/ explicit propagate_const(propagate_const<U>&& p) noexcept(std::is_nothrow_constructible<T, U&&>::value) :
        t(std::move(p.t)) {
    }
    template <typename U, std::enable_if_t<
        std::is_convertible<U&&, T>::value, int> = 0>
    /*constexpr*/ propagate_const& operator=(propagate_const<U>&& p) noexcept(std::is_nothrow_assignable<T, U&&>::value) {
        t = std::move(p.t);
        return *this;
    }

    template <typename U, std::enable_if_t<
        !detail::is_propagate_const<std::decay_t<U>>::value &&
        std::is_constructible<T, U&&>::value &&
        std::is_convertible<U&&, T>::value, int> = 0>
    /*constexpr*/ propagate_const(U&& p) noexcept(std::is_nothrow_constructible<T, U&&>::value) :
        t(std::forward<U>(p)) {
    }
    template <typename U, std::enable_if_t<
        !detail::is_propagate_const<std::decay_t<U>>::value &&
        std::is_constructible<T, U&&>::value &&
        !std::is_convertible<U&&, T>::value, int> = 0>
    /*constexpr*/ explicit propagate_const(U&& p) noexcept(std::is_nothrow_constructible<T, U&&>::value) :
        t(std::forward<U>(p)) {
    }
    template <typename U, std::enable_if_t<
        !detail::is_propagate_const<std::decay_t<U>>::value &&
        std::is_convertible<U&&, T>::value, int> = 0>
    /*constexpr*/ propagate_const& operator=(U&& p) noexcept(std::is_nothrow_assignable<T, U&&>::value) {
        t = std::forward<U>(p);
        return *this;
    }

    template <typename T_ = T, std::enable_if_t<
        std::is_constructible<bool, T_>::value &&
        !std::is_convertible<T_, bool>::value, int> = 0>
    constexpr explicit operator bool() const noexcept(std::is_nothrow_constructible<bool, T>::value) { return bool{t}; }

    template <typename T_ = T, std::enable_if_t<
        std::is_constructible<bool, T_>::value &&
        !std::is_convertible<T_, bool>::value, int> = 0>
    /*constexpr*/ explicit operator bool() noexcept(std::is_nothrow_constructible<bool, T>::value) { return bool{t}; }

    template <typename T_ = T, std::enable_if_t<
        std::is_same<pointer_type, decltype(std::declval<T_>().get())>::value, int> = 0>
    constexpr const_pointer_type get() const noexcept(noexcept(get_pointer(t)) && std::is_nothrow_move_constructible<const_pointer_type>::value) { return get_pointer(t); }
    template <typename T_ = T, std::enable_if_t<
        std::is_same<pointer_type, decltype(std::declval<T_>().get())>::value, int> = 0>
    /*constexpr*/ pointer_type get() noexcept(noexcept(get_pointer(t)) && std::is_nothrow_move_constructible<pointer_type>::value) { return get_pointer(t); }

    constexpr const_reference_type operator*() const noexcept(noexcept(*get_pointer(t)) && std::is_nothrow_move_constructible<const_reference_type>::value) { return *get_pointer(t); }
    /*constexpr*/ reference_type operator*() noexcept(noexcept(*get_pointer(t)) && std::is_nothrow_move_constructible<reference_type>::value) { return *get_pointer(t); }

    constexpr const_pointer_type operator->() const noexcept(noexcept(get_pointer(t)) && std::is_nothrow_move_constructible<const_pointer_type>::value) { return get_pointer(t); }
    /*constexpr*/ pointer_type operator->() noexcept(noexcept(get_pointer(t)) && std::is_nothrow_move_constructible<pointer_type>::value) { return get_pointer(t); }

    /*constexpr*/ operator T&() & noexcept { return t; }
    /*constexpr*/ operator T() && noexcept(std::is_nothrow_move_constructible<T>::value) { return std::move(t); }

    template <typename T_ = T, std::enable_if_t<
        std::is_same<T_, const_type>::value, int> = 0>
    constexpr operator const_type const&() const& noexcept {
        return t;
    }
    template <typename T_ = T, std::enable_if_t<
        !std::is_same<T_, const_type>::value, int> = 0>
    constexpr operator const_type() const& noexcept(std::is_nothrow_constructible<const_type, T const>::value) {
        return t;
    }

    template <typename T_ = T, std::enable_if_t<
        !std::is_same<T_, const_type>::value, int> = 0>
    /*constexpr*/ operator const_type() && noexcept(std::is_nothrow_constructible<const_type, T&&>::value) {
        return std::move(t);
    }

    template <typename T_ = T, std::enable_if_t<
        std::is_convertible<T_, pointer_type>::value &&
        !std::is_same<pointer_type, T_>::value &&
        !std::is_same<pointer_type, const_type>::value, int> = 0>
    /*constexpr*/ operator pointer_type() & noexcept(std::is_nothrow_constructible<pointer_type, T>::value) { return t; }
    template <typename T_ = T, std::enable_if_t<
        std::is_convertible<T_, pointer_type>::value &&
        !std::is_same<pointer_type, T_>::value &&
        !std::is_same<pointer_type, const_type>::value, int> = 0>
    /*constexpr*/ operator pointer_type() && noexcept(std::is_nothrow_constructible<pointer_type, T&&>::value) { return std::move(t); }

    template <typename T_ = T, std::enable_if_t<
        std::is_constructible<pointer_type, T_>::value &&
        !std::is_convertible<T_, pointer_type>::value &&
        !std::is_same<pointer_type, T_>::value &&
        !std::is_same<pointer_type, const_type>::value, int> = 0>
    /*constexpr*/ explicit operator pointer_type() & noexcept(std::is_nothrow_constructible<pointer_type, T>::value && std::is_nothrow_move_constructible<pointer_type>::value) { return static_cast<pointer_type>(t); }
    template <typename T_ = T, std::enable_if_t<
        std::is_constructible<pointer_type, T_>::value &&
        !std::is_convertible<T_, pointer_type>::value &&
        !std::is_same<pointer_type, T_>::value &&
        !std::is_same<pointer_type, const_type>::value, int> = 0>
    /*constexpr*/ explicit operator pointer_type() && noexcept(std::is_nothrow_constructible<pointer_type, T&&>::value && std::is_nothrow_move_constructible<pointer_type>::value) { return static_cast<pointer_type>(std::move(t)); }

    template <typename T_ = T, std::enable_if_t<
        std::is_convertible<T_, reference_type>::value, int> = 0>
    /*constexpr*/ operator reference_type() noexcept(std::is_nothrow_constructible<T, reference_type>::value) { return t; }

    template <typename T_ = T, std::enable_if_t<
        std::is_constructible<reference_type, T_>::value &&
        !std::is_convertible<T_, reference_type>::value, int> = 0>
    /*constexpr*/ explicit operator reference_type() noexcept(std::is_nothrow_constructible<reference_type, T>::value && std::is_nothrow_move_constructible<reference_type>::value) { return static_cast<reference_type>(t); }

    template <typename T_ = T, std::enable_if_t<
        std::is_convertible<T_ const, const_pointer_type>::value &&
        !std::is_same<const_pointer_type, T_>::value &&
        !std::is_same<const_pointer_type, const_type>::value, int> = 0>
    constexpr operator const_pointer_type() const& noexcept(std::is_nothrow_constructible<const_pointer_type, T>::value && std::is_nothrow_move_constructible<const_pointer_type>::value) { return t; }

    template <typename T_ = T, std::enable_if_t<
        std::is_constructible<const_pointer_type, T_ const>::value &&
        !std::is_convertible<T_ const, const_pointer_type>::value &&
        !std::is_same<const_pointer_type, T_>::value &&
        !std::is_same<const_pointer_type, const_type>::value, int> = 0>
    constexpr explicit operator const_pointer_type() const& noexcept(std::is_nothrow_constructible<const_pointer_type, T>::value && std::is_nothrow_move_constructible<const_pointer_type>::value) { return static_cast<const_pointer_type>(t); }

    template <typename T_ = T, std::enable_if_t<
        std::is_convertible<T_ const, const_reference_type>::value, int> = 0>
    constexpr operator const_reference_type() const noexcept(std::is_nothrow_constructible<const_reference_type, T>::value) { return t; }

    template <typename T_ = T, std::enable_if_t<
        std::is_constructible<const_reference_type, T_>::value &&
        !std::is_convertible<T_, const_reference_type>::value, int> = 0>
    constexpr explicit operator const_reference_type() const noexcept(std::is_nothrow_constructible<const_reference_type, T>::value && std::is_nothrow_move_constructible<const_reference_type>::value) { return static_cast<const_reference_type>(t); }

    /*constexpr*/ void swap(propagate_const& other) noexcept(noexcept(detail::swap_(t, other.t))) {
        detail::swap_(t, other.t);
    }
};

template <typename T>
/*constexpr*/ void swap(propagate_const<T>& lhs, propagate_const<T>& rhs) noexcept(noexcept(lhs.swap(rhs))) {
    lhs.swap(rhs);
}

template <typename T>
constexpr T const& get_underlying(propagate_const<T> const& pc) noexcept {
    return const_cast<propagate_const<T>&>(pc);
}
template <typename T>
constexpr T& get_underlying(propagate_const<T>& pc) noexcept {
    return pc;
}

template <typename T, typename P = typename propagate_const<T>::const_pointer_type>
constexpr P get_pointer(propagate_const<T> const& pc) noexcept(noexcept(P{static_cast<P>(pc)})) {
    return static_cast<typename propagate_const<T>::const_pointer_type>(pc);
}

template <typename T, typename P = typename propagate_const<T>::pointer_type>
constexpr P get_pointer(propagate_const<T>& pc) noexcept(noexcept(P{static_cast<P>(pc)})) {
    return static_cast<P>(pc);
}

template <typename T1, typename T2, typename = std::enable_if_t<!std::is_same<T1, T2>::value>>
constexpr bool operator==(propagate_const<T1> const& lhs, T2 const& rhs) noexcept(noexcept(get_underlying(lhs) == rhs)) { return get_underlying(lhs) == rhs; }
template <typename T1, typename T2, typename = std::enable_if_t<!std::is_same<T1, T2>::value>>
constexpr bool operator==(T1 const& lhs, propagate_const<T2> const& rhs) noexcept(noexcept(lhs == get_underlying(rhs))) { return lhs == get_underlying(rhs); }
template <typename T1, typename T2, typename = std::enable_if_t<!std::is_same<T1, T2>::value>>
constexpr bool operator!=(propagate_const<T1> const& lhs, T2 const& rhs) noexcept(noexcept(get_underlying(lhs) != rhs)) { return get_underlying(lhs) != rhs; }
template <typename T1, typename T2, typename = std::enable_if_t<!std::is_same<T1, T2>::value>>
constexpr bool operator!=(T1 const& lhs, propagate_const<T2> const& rhs) noexcept(noexcept(lhs != get_underlying(rhs))) { return lhs != get_underlying(rhs); }

template <typename T1, typename T2>
constexpr bool operator==(propagate_const<T1> const& lhs, propagate_const<T2> const& rhs) noexcept(noexcept(get_underlying(lhs) == get_underlying(rhs))) { return get_underlying(lhs) == get_underlying(rhs); }
template <typename T1, typename T2>
constexpr bool operator!=(propagate_const<T1> const& lhs, propagate_const<T2> const& rhs) noexcept(noexcept(get_underlying(lhs) != get_underlying(rhs))) { return get_underlying(lhs) != get_underlying(rhs); }

template <typename T1, typename T2>
constexpr bool operator<(propagate_const<T1> const& lhs, propagate_const<T2> const& rhs) noexcept(noexcept(get_underlying(lhs) < get_underlying(rhs))) { return get_underlying(lhs) < get_underlying(rhs); }
template <typename T1, typename T2>
constexpr bool operator<=(propagate_const<T1> const& lhs, propagate_const<T2> const& rhs) noexcept(noexcept(get_underlying(lhs) <= get_underlying(rhs))) { return get_underlying(lhs) <= get_underlying(rhs); }
template <typename T1, typename T2>
constexpr bool operator>(propagate_const<T1> const& lhs, propagate_const<T2> const& rhs) noexcept(noexcept(get_underlying(lhs) > get_underlying(rhs))) { return get_underlying(lhs) > get_underlying(rhs); }
template <typename T1, typename T2>
constexpr bool operator>=(propagate_const<T1> const& lhs, propagate_const<T2> const& rhs) noexcept(noexcept(get_underlying(lhs) >= get_underlying(rhs))) { return get_underlying(lhs) >= get_underlying(rhs); }

namespace std {

template <typename T>
struct equal_to<propagate_const<T>> {
    constexpr bool operator()(propagate_const<T> const& lhs, propagate_const<T> const& rhs) const noexcept(noexcept(std::equal_to<T>()(get_underlying(lhs), get_underlying(rhs)))) {
        return std::equal_to<T>()(get_underlying(lhs), get_underlying(rhs));
    }
};
template <typename T>
struct not_equal_to<propagate_const<T>> {
    constexpr bool operator()(propagate_const<T> const& lhs, propagate_const<T> const& rhs) const noexcept(noexcept(std::not_equal_to<T>()(get_underlying(lhs), get_underlying(rhs)))) {
        return std::not_equal_to<T>()(get_underlying(lhs), get_underlying(rhs));
    }
};
template <typename T>
struct less<propagate_const<T>> {
    constexpr bool operator()(propagate_const<T> const& lhs, propagate_const<T> const& rhs) const noexcept(noexcept(std::less<T>()(get_underlying(lhs), get_underlying(rhs)))) {
        return std::less<T>()(get_underlying(lhs), get_underlying(rhs));
    }
};
template <typename T>
struct less_equal<propagate_const<T>> {
    constexpr bool operator()(propagate_const<T> const& lhs, propagate_const<T> const& rhs) const noexcept(noexcept(std::less_equal<T>()(get_underlying(lhs), get_underlying(rhs)))) {
        return std::less_equal<T>()(get_underlying(lhs), get_underlying(rhs));
    }
};
template <typename T>
struct greater<propagate_const<T>> {
    constexpr bool operator()(propagate_const<T> const& lhs, propagate_const<T> const& rhs) const noexcept(noexcept(std::greater<T>()(get_underlying(lhs), get_underlying(rhs)))) {
        return std::greater<T>()(get_underlying(lhs), get_underlying(rhs));
    }
};
template <typename T>
struct greater_equal<propagate_const<T>> {
    constexpr bool operator()(propagate_const<T> const& lhs, propagate_const<T> const& rhs) const noexcept(noexcept(std::greater_equal<T>()(get_underlying(lhs), get_underlying(rhs)))) {
        return std::greater_equal<T>()(get_underlying(lhs), get_underlying(rhs));
    }
};

template <typename T>
struct hash<propagate_const<T>> {
    constexpr std::size_t operator()(propagate_const<T> const& pc) const noexcept(noexcept(hash<T>()(get_underlying(pc)))) {
        return hash<T>()(get_underlying(pc));
    }
};

} // namespace std

#endif // PROPAGATE_CONST_HPP
