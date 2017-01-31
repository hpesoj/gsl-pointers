# `observer` and `observer_ptr`: high-level non-owning pointer-like types for C++

This is a variation on proposal [WG31 N4282, "A Proposal for the World’s Dumbest Smart Pointer, v4"](http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2014/n4282.pdf), which proposes the creation of the [`observer_ptr`](http://en.cppreference.com/w/cpp/experimental/observer_ptr) non-owning "smart" pointer class template. This proposal:

* Adds `observer` as a null-less counterpart to `observer_ptr`
* Allows implicit conversion from `T&` to `observer` and `observer_ptr`
* Changes the `get` member function to a non-member `get_pointer` function
* Makes a number of other tweaks

Included is a [reference implementation](api/observer.hpp) and a full [test suite](tests/observer_tests.cpp).

## Rationale

### Why `observer<T>`?

The `observer_ptr<T>` class template does a great job of addressing the deficiencies of `T*` when used as a non-owning "observer", by providing a minimal interface that documents its purpose (as a so-called "vocabulary type"). However, `observer_ptr<T>` has a null state, which is great if you need to be able to indicate that "nothing" is being observed, but this is not always required or even desirable. For example, one may have a container of observed objects:

    vector<observer_ptr<T>> obs;

In this case, we simply remove an item from the container to stop observing it: there is no need for the null state. In fact, we would prefer to be able to iterate over the `vector` without worrying about null pointers:

    for (auto o : obs) {
        frobnicate(*o); // precondition: `o != nullptr`
    }

If a null pointer somehow finds its way into the container, then we have undefined behaviour on our hands. The `observer<T>` class template is the same as `observer_ptr<T>`, but without a null state; by using it, we eliminate the possibility of null pointer bugs at completely:

    vector<observer<T>> obs;
    
    obs.emplace_back();        // error: no such constructor `observer<T>()`
    obs.emplace_back(nullptr); // error: no such constructor `observer<T>(nullptr_t)`
    obs.emplace_back(&t);      // error: no such constructor `observer<T>(T*)`

    obs.emplace_back(t);       // okay: calls constructor `observer<T>(T&)`

    for (auto o : obs) {
        frobnicate(*o); // `o` cannot be null
    }

#### Why not `T&`?

Lvalue references are sometime used where a pointers would otherwise be used, because references cannot be null. However, references are a poor substitute for pointers; both have their uses, but they are semantically and functionally distinct.

Firstly, references and pointers have _value_ and _reference_ assignment semantics respectively. In other words, assigning to a reference modifies the referenced _value_, while assigning to a pointer changes what it _references_. While a pointer can be dereferenced to modify the referenced value, there is no way to change what a reference references. This also means that classes with reference data members are _non-copy assignable_ by default.

Secondly, you cannot have arrays of references or store references in STL containers. This is because references are not object types, which means it is not possible to take the address of a reference. Of course, it is possible to use `reference_wrapper` to get the effect a container of references, as it has _reference_ assignment semantics like a pointer. However, `reference_wrapper` has value _comparison_ semantics, like a reference, so it interacts very differently with STL algorithms and containers.

Lastly, references as "observer" function parameters do a poor job of conveying intent. A function receiving such an argument is almost certainly going to copy it into persistent state, but reference parameters are also commonly used to avoid expensive or impossible copies or as "out" parameters, so this is not entirely clear just from looking at the function signature. Const reference parameters can even receive _temporary_ arguments, and storing a persistent reference to a temporary object is a very bad idea indeed.

### Why conversion from `T&`?

Construction of an owning smart pointer from a reference would be incorrect. Ownership in C++ is conveyed by pointer, not by reference: `new` returns a pointer and `delete` takes a pointer. But `observer<T>` and `observer_ptr<T>` do not take ownership of what they reference, so it is perfectly safe for them to be constructible from `T&`. In fact, unlike pointers, in a well-defined program, references _always_ refer to individual valid objects. Thus, it is safe to allow _implicit conversion_ from `T&` to `observer<T>` or `observer_ptr<T>`. Note that conversion from `T&&` is deleted for both types, since it is not possible to take the address of an rvalue.

### Why `get_pointer`?

Unlike `unique_ptr<T>` and `shared_ptr<T>`, neither `observer<T>` nor `observer_ptr<T>` define the member function `get` to return a pointer to what they hold. We feel that the name `get` is not descriptive enough, as `observer<T>` lacks the `_ptr` suffix. Indeed, given that `observer<T>` is convertible from `T&`, users might expect a `get` function to return a reference (like `reference_wrapper<T>`). Instead, we feel that, for a pointer-like type, it is natural to enable _explicit_ conversion to `T*`, and then provide a free function called `get_pointer` for convenience (casting requires a pointer type to be specified).

Though it isn't part of this proposal, we suggest that this would be a more natural approach for _all_ pointer-like types, and recommend that `get_pointer` overloads be added for `T*`, `unique_ptr<T>` and `shared_ptr<T>` (whether all smart pointers should support explicit conversion to `T*` is another question). This will facilitate generic programming with pointer-like types, much like how the `begin` and `end` free functions have facilitated generic programming with ranges. One actual example is the proposed `propagate_const<T>` interface adapter type, which currently imposes the requirement that compatible class types have a `get` member function. Such a requirement is unnecessarily intrusive. By instead depending on `get_pointer`, `propagate_const` can avoid forcing changes to pointer-like types that are not already compatible, something that the user may not even have the capacity to do.
