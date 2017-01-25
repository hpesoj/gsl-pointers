# `observer` and `observer_ptr`: high-level non-owning pointer-like types for C++

This is a variation on proposal [WG31 N4282, "A Proposal for the World’s Dumbest Smart Pointer, v4"](http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2014/n4282.pdf), which proposes the creation of the [`observer_ptr`](http://en.cppreference.com/w/cpp/experimental/observer_ptr) non-owning "smart" pointer class template. This proposal:

* Adds `observer` as a null-less counterpart to `observer_ptr`
* Allows implicit conversion from `T&` to `observer` and `observer_ptr`
* Defines the `const_type` member type for `observer` and `observer_ptr`
* Changes the `get` member function to a non-member `get_pointer` function
* Alters `make_observer` to take `T&` and return `observer<T>`
* Makes a number of other tweaks

Included is a [reference implementation](api/observer.hpp) and a full [test suite](tests/observer_tests.cpp).
