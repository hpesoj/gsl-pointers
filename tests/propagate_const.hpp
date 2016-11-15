#ifndef PROPAGATE_CONST_HPP
#define PROPAGATE_CONST_HPP

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

} // namespace detail

template <typename T>
T* get_pointer(T* p) { return p; }

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

    propagate_const() = default;

    propagate_const(propagate_const const&) = delete;
    propagate_const& operator=(propagate_const const&) = delete;
    propagate_const(propagate_const&&) = default;
    propagate_const& operator=(propagate_const&&) = default;

    template <typename U, std::enable_if_t<
        std::is_constructible<T, U&&>::value &&
        std::is_convertible<U&&, T>::value, int> = 0>
    propagate_const(propagate_const<U>&& p) :
        t(std::move(p)) {
    }
    template <typename U, std::enable_if_t<
        std::is_constructible<T, U&&>::value &&
        !std::is_convertible<U&&, T>::value, int> = 0>
    explicit propagate_const(propagate_const<U>&& p) :
        t(std::move(p.t)) {
    }
    template <typename U, std::enable_if_t<
        std::is_convertible<U&&, T>::value, int> = 0>
    propagate_const& operator=(propagate_const<U>&& p) {
        t = std::move(p);
        return *this;
    }

    template <typename U, std::enable_if_t<
        !detail::is_propagate_const<std::decay_t<U>>::value &&
        std::is_constructible<T, U&&>::value &&
        std::is_convertible<U&&, T>::value, int> = 0>
    propagate_const(U&& p) :
        t(std::forward<U>(p)) {
    }
    template <typename U, std::enable_if_t<
        !detail::is_propagate_const<std::decay_t<U>>::value &&
        std::is_constructible<T, U&&>::value &&
        !std::is_convertible<U&&, T>::value, int> = 0>
    explicit propagate_const(U&& p) :
        t(std::forward<U>(p)) {
    }
    template <typename U, std::enable_if_t<
        !detail::is_propagate_const<std::decay_t<U>>::value &&
        std::is_convertible<U&&, T>::value, int> = 0>
    propagate_const& operator=(U&& p) {
        t = std::forward<U>(p);
        return *this;
    }

    template <typename T_ = T, std::enable_if_t<
        std::is_convertible<T_, bool>::value, int> = 0>
    explicit operator bool() const { return static_cast<bool>(t); }

    template <typename T_ = T, std::enable_if_t<
        std::is_same<pointer_type, decltype(std::declval<T_>().get())>::value, int> = 0>
    const_pointer_type get() const { return get_pointer(t); }
    template <typename T_ = T, std::enable_if_t<
        std::is_same<pointer_type, decltype(std::declval<T_>().get())>::value, int> = 0>
    pointer_type get() { return get_pointer(t); }

    const_reference_type operator*() const { return *get_pointer(t); }
    reference_type operator*() { return *get_pointer(t); }

    const_pointer_type operator->() const { return get_pointer(t); }
    pointer_type operator->() { return get_pointer(t); }

    operator T&() & { return t; }
    operator T() && { return std::move(t); }
    operator T() const&& { return std::move(t); }

    template <typename T_ = T, std::enable_if_t<
        std::is_same<T_, const_type>::value, int> = 0>
    operator const_type const&() const& {
        return t;
    }
    template <typename T_ = T, std::enable_if_t<
        !std::is_same<T_, const_type>::value, int> = 0>
    operator const_type() const& {
        return t;
    }

    template <typename T_ = T, std::enable_if_t<
        !std::is_same<T_, const_type>::value, int> = 0>
    operator const_type() && {
        return std::move(t);
    }
    template <typename T_ = T, std::enable_if_t<
        !std::is_same<T_, const_type>::value, int> = 0>
    operator const_type() const&& {
        return std::move(t);
    }

    template <typename T_ = T, std::enable_if_t<
        std::is_convertible<T_, pointer_type>::value &&
        !std::is_same<pointer_type, T_>::value &&
        !std::is_same<pointer_type, const_type>::value, int> = 0>
    operator pointer_type() & { return t; }
    template <typename T_ = T, std::enable_if_t<
        std::is_convertible<T_, pointer_type>::value &&
        !std::is_same<pointer_type, T_>::value &&
        !std::is_same<pointer_type, const_type>::value, int> = 0>
    operator pointer_type() && { return std::move(t); }

    template <typename T_ = T, std::enable_if_t<
        std::is_constructible<pointer_type, T_>::value &&
        !std::is_convertible<T_, pointer_type>::value &&
        !std::is_same<pointer_type, T_>::value &&
        !std::is_same<pointer_type, const_type>::value, int> = 0>
    explicit operator pointer_type() & { return static_cast<pointer_type>(t); }
    template <typename T_ = T, std::enable_if_t<
        std::is_constructible<pointer_type, T_>::value &&
        !std::is_convertible<T_, pointer_type>::value &&
        !std::is_same<pointer_type, T_>::value &&
        !std::is_same<pointer_type, const_type>::value, int> = 0>
    explicit operator pointer_type() && { return std::move(static_cast<pointer_type>(t)); }

    template <typename T_ = T, std::enable_if_t<
        std::is_convertible<T_, reference_type>::value, int> = 0>
    operator reference_type() { return t; }

    template <typename T_ = T, std::enable_if_t<
        std::is_convertible<T_ const, const_pointer_type>::value &&
        !std::is_same<const_pointer_type, T_>::value &&
        !std::is_same<const_pointer_type, const_type>::value, int> = 0>
    operator const_pointer_type() const& { return t; }
    template <typename T_ = T, std::enable_if_t<
        std::is_convertible<T_ const, const_pointer_type>::value &&
        !std::is_same<const_pointer_type, T_>::value &&
        !std::is_same<const_pointer_type, const_type>::value, int> = 0>
    operator const_pointer_type() const&& { return std::move(t); }

    template <typename T_ = T, std::enable_if_t<
        std::is_constructible<const_pointer_type, T_ const>::value &&
        !std::is_convertible<T_ const, const_pointer_type>::value &&
        !std::is_same<const_pointer_type, T_>::value &&
        !std::is_same<const_pointer_type, const_type>::value, int> = 0>
    explicit operator const_pointer_type() const& { return static_cast<const_pointer_type>(t); }
    template <typename T_ = T, std::enable_if_t<
        std::is_constructible<const_pointer_type, T_ const>::value &&
        !std::is_convertible<T_ const, const_pointer_type>::value &&
        !std::is_same<const_pointer_type, T_>::value &&
        !std::is_same<const_pointer_type, const_type>::value, int> = 0>
    explicit operator const_pointer_type() const&& { return std::move(static_cast<const_pointer_type>(t)); }

    template <typename T_ = T, std::enable_if_t<
        std::is_convertible<T_ const, const_reference_type>::value, int> = 0>
    operator const_reference_type() const { return t; }
};

template <typename T>
constexpr typename propagate_const<T>::pointer_type get_pointer(propagate_const<T>& pc) noexcept {
    return static_cast<typename propagate_const<T>::pointer_type>(pc);
}

template <typename T>
constexpr typename propagate_const<T>::const_pointer_type get_pointer(propagate_const<T> const& pc) noexcept {
    return static_cast<typename propagate_const<T>::const_pointer_type>(pc);
}

#endif // PROPAGATE_CONST_HPP
