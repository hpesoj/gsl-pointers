# `observer` and `observer_ptr`: high-level non-owning pointer-like types for C++

This is a variation on proposal [WG31 N4282, "A Proposal for the World’s Dumbest Smart Pointer, v4"](http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2014/n4282.pdf), which proposes the creation of the [`observer_ptr`](http://en.cppreference.com/w/cpp/experimental/observer_ptr) non-owning "smart" pointer class template. This proposal:

* Adds `observer` as a null-less counterpart to `observer_ptr`
* Allows implicit conversion from `T&` to `observer` and `observer_ptr`
* Defines the `const_type` member type for `observer` and `observer_ptr`
* Changes the `get` member function to a non-member `get_pointer` function
* Alters `make_observer` to take `T&` and return `observer<T>`
* Makes a number of other tweaks

Included is a [reference implementation](api/observer.hpp) and a full [test suite](tests/observer_tests.cpp).

## Rationale

### Why `observer`?

The `observer_ptr<T>` class template does a great job of addressing the deficiencies of `T*` when used as a non-owning "observer" of single objects, by providing a narrower interface that only supports operations that make sense for purpose. However, `observer_ptr<T>` still has a null state, which is great if you need to be able to indicate that "nothing" is being observed, but this is not always required. A good example is when storing a `vector` of `observer_ptr<T>`s:

    vector<observer_ptr<T>> observers;

In this case, we simply remove an item from the array to stop observing it: there is no need for the null state. In fact, we would prefer to be able to iterate over the `vector` without worrying about null pointers:

    for (auto o : observers) {
        frobnicate(*o);
    }

If a null pointer somehow finds its way into the `vector`, then we have undefined behaviour on our hands. The `observer<T>` class template is the same as `observer_ptr<T>`, but without a null state. By using `observer<T>`, we prevent such null pointer bugs at _compile-time_ rather than waiting until run-time to experience the unpleasant effects of undefined behaviour:

    vector<observer<T>> observers;
    
    observers.emplace_back();        // error: no such constructor `observer<T>()`
    observers.emplace_back(nullptr); // error: no such constructor `observer<T>(nullptr_t)`
    observers.emplace_back(&t);      // error: no such constructor `observer<T>(T*)`
