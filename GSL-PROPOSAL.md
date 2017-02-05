# Proposal to add `observer` to the GSL

## Abstract

This document makes the case for the inclusion of non-owning pointer-like type `observer<T>` in the [Guideline Support Library](https://github.com/Microsoft/GSL), and its subsequent incorporation into the [C++ Core Guidelines](https://github.com/isocpp/CppCoreGuidelines). It also makes a case against encouraging the use of `T*` in high-level modern C++ code.

## Introduction

The `observer<T>` class template is a pointer-like type designed to replace `T*` wherever it is used as a non-owning "observer" of an object of type `T`.

    observer<T> o = make_observer(t);

`observer<T>` is more than just a non-owning reference: it conveys the _intent_ to store the reference to be "observed" later on. That is, in contrast to `unique_ptr<T>` and `shared_ptr<T>`, which convey _ownership_, `observer<T>` conveys _observership_.

`observer<T>` does not manage or track the lifetime of what it observes. It is designed to have no more overhead than a simple pointer, and requires that the lifetimes of observers and observees are managed further up the call stack. If automatic lifetime tracking is required, a signals and slots implementation (e.g. [Boost.Signals2](www.boost.org/doc/libs/release/doc/html/signals2.html)) or `weak_ptr` may be used.

## Examples

`observer<T>` can be used to implement the [observer pattern](https://en.wikipedia.org/wiki/Observer_pattern):

    wgt.register_callback(make_observer(my_callback));

In this example, `register_callback` takes an `observer` parameter to indicate its intent to store a reference to the callback for later use. The caller is required to use `make_observer` and in doing so is explicitly made aware of the fact that `register_callback` is going to store a reference to `my_callback` for later use. This is a solution to the [proto-rule](https://github.com/isocpp/CppCoreGuidelines/blob/master/CppCoreGuidelines.md#S-unclassified), "Never pass a pointer down the call stack" because you never know who is going to store it. Well now you do!

Despite its name, use cases for `observer<T>` are not limited to observer pattern implementations. For example, you could construct a tree of nodes:

    struct tree_node {
        optional<observer<tree_node>> parent;
        vector<observer<tree_node>> children;
    }; 

## Why not `T*`?

Rule [R.3]([R.3](https://github.com/isocpp/CppCoreGuidelines/blob/master/CppCoreGuidelines.md#r3-a-raw-pointer-a-t-is-non-owning)) states that, _"A raw pointer (a `T*`) is non-owning"_, explaining that:

> There is nothing (in the C++ standard or in most code) to say otherwise and most raw pointers are non-owning.

This is a reasonable assertion to make, given that rule [P.1](https://github.com/isocpp/CppCoreGuidelines/blob/master/CppCoreGuidelines.md#Rp-direct) advises us to, _"Express ideas directly in code"_, noting that:

> Compilers don't read comments (or design documents) and neither do many programmers (consistently). What is expressed in code has defined semantics and can (in principle) be checked by compilers and other tools.

The type of an object, `T*`, by itself conveys no information about how the programmer should manage the lifetime of the resource to which the object points. Thus, it is sensible to assume that, unless indicated otherwise, an object of type `T*` is non-owning.

Given the advice to, "Express ideas in code", along with rule [I.4](https://github.com/isocpp/CppCoreGuidelines/blob/master/CppCoreGuidelines.md#Ri-typed), which tells us to, _"Make interfaces precisely and strongly typed"_, explaining that:

> Types are the simplest and best documentation, have well-defined meaning, and are guaranteed to be checked at compile-time.

We can consider rule [F.22](https://github.com/isocpp/CppCoreGuidelines/blob/master/CppCoreGuidelines.md#f22-use-t-or-ownert-to-designate-a-single-object), which advises us to, _"Use `T*` or `owner<T*>` to designate a single object"_, giving the reasoning:

> Readability: it makes the meaning of a plain pointer clear. Enables significant tool support.

The `owner<T>` type alias uses the type system—_the best documentation there is_—to mark _directly in code_ that an object of type `T` should be explicitly `delete`'d before it goes out of scope. This is a great approach that will enable tools to detect memory leaks and other bugs, _and_ it is helpful to the programmer.

However, this rule also suggests that `T*` designates a reference to a "single object". This is simply not true. In fact, `T*` designates a [_random access iterator_](http://en.cppreference.com/w/cpp/concept/RandomAccessIterator) pointing to a sequence of objects of type `T*` in contiguous memory:

    int values[] = { 1, 2, 3 };
    int* b = begin(values);
    int* e = end(values);

A `T*` that points to a "single object" is simply an iterator to the beginning of a sequence of objects of length _one_. Similarly, a `T*` that points to an "array of objects" is simply an iterator that points to the _beginning_ of a sequence of objects of length _greater than or equal to_ one.

    T* p1 = &obj; 
    T* p2 = arr;
    T* p3 = begin(arr);

Here, `p1`, `p2` and `p3` are _all_ iterators, and could all legitimately be used as inputs to various STL algorithms:

    for_each(p, p + 1, f);

In modern C++, the type system _tells_ us what an object is, and `T*` tells us it is a random access iterator.

So if every `T*` is an iterator, what do references to single objects and arrays look like in modern C++?

A reference to (or "view" of) an object looks like this:

    T&

A reference to (or "view" of) a contiguous sequence of objects looks like this:

    span<T>

Sequences of objects have traditionally been represented by a pairs of iterators `T*` iterators (many STL algorithms take pairs of iterators), but rule .

## What about null pointers?




Currently, the C++ Core Guidelines state that it is appropriate to use raw pointers as observers (see rules [F.7](https://github.com/isocpp/CppCoreGuidelines/blob/master/CppCoreGuidelines.md#Rf-smart), [F.22](https://github.com/isocpp/CppCoreGuidelines/blob/master/CppCoreGuidelines.md#f22-use-t-or-ownert-to-designate-a-single-object), [F.60](https://github.com/isocpp/CppCoreGuidelines/blob/master/CppCoreGuidelines.md#f60-prefer-t-over-t-when-no-argument-is-a-valid-option) and [R.3](https://github.com/isocpp/CppCoreGuidelines/blob/master/CppCoreGuidelines.md#r3-a-raw-pointer-a-t-is-non-owning)). This section outlines why this is bad advice that violates some of the guidelines' own rules.

Both `observer<T>` and `observer_ptr<T>` are superior to `T*` in the capacity of an observer for three main reasons:

1. __Type safety__ – Neither `observer_ptr<T>` nor `observer<T>` are implicitly convertible from or to `T*`, because implicit operations should be type-safe, and `T*` can represent things other than an observer (e.g. array, iterator), and do not even _have_ to point to a valid object (e.g. a past-the-end iterator). Instead, observers are created from `T&` using the type-safe `make_observer` factory function, since (in a well-formed program) `T&` is _always_ a non-owning reference to a valid object.
2. __Documentation of intent__ – It is 100% clear that `observer<T>` and `observer_ptr<T>` represent mandatory and optional "observers" respectively, and will be stored for access later on. Conversely, it is not clear whether `T*` can or cannot be null, or whether it will be copied and stored.
3. __Interface safety__ – Both `observer<T>` and `observer_ptr<T>` have minimal interfaces that implement only operations that make sense for an observer. Conversely, `T*` support operations that do not make sense for an observer (e.g. pointer arithmetic operators, array subscript operator, conversion to `void*`). In addition, `observer<T>` enforces the "not null" precondition at _compile-time_.

### `T*` is not strongly typed

Rule [I.4](https://github.com/isocpp/CppCoreGuidelines/blob/master/CppCoreGuidelines.md#Ri-typed) tells us to, "Make interfaces precisely and strongly typed", explaining that:

> Types are the simplest and best documentation, have well-defined meaning, and are guaranteed to be checked at compile-time.

An interface taking a `T*` violates this rules because `T*` is weakly typed, because it can represent several different things:

* `T*` can be an object
* `T*` can be an array
* `T*` can be an iterator

And there are further variations of these things:

* If `T*` points to an object or an array that has been allocated on the free store, it carries no information about how or when the object or array should be _deallocated_. Deallocating an object or array incorrectly can result in _undefined behaviour_ or a _resource leak_.
* If `T*` points to an array, it carries no information about the _size_ of the array. The array may also be zero-terminated, but `T*` carries no information to indicate this either. Out of bounds array access results in _undefined behaviour_. 
* In all cases, `T*` also carries no information about whether or not it can validly be _null_. C++ emphasises performance, so it is often desirable to assume that a pointer is not null to avoid the run-time cost incurred by a conditional check. Dereferencing a null pointer results in _undefined behaviour_.

All of these things are conceptually distinct, but they all implicitly convert to each other without the compiler complaining because they are all represented by the same type.

Of course, the C++ Core Guidelines provide alternatives to `T*` so that the safety and clarity of code can be improved.

Rule [R.3]([R.3](https://github.com/isocpp/CppCoreGuidelines/blob/master/CppCoreGuidelines.md#r3-a-raw-pointer-a-t-is-non-owning)) states that, _"A raw pointer (a `T*`) is non-owning"_, explaining that:

> There is nothing (in the C++ standard or in most code) to say otherwise and most raw pointers are non-owning.

Rule [F.22](https://github.com/isocpp/CppCoreGuidelines/blob/master/CppCoreGuidelines.md#f22-use-t-or-ownert-to-designate-a-single-object) advises us to, _"Use `T*` or `owner<T*>` to designate a single object"_, giving the reasoning:

> Readability: it makes the meaning of a plain pointer clear. Enables significant tool support.

 The argument is that, once all other uses of `T*` have been replaced with alternatives, all instances of `T*` will be non-owning references to single objects. This argument assumes that:

1. Everyone is following the guidelines perfectly.
2. Every part of a code-base adheres to the guidelines.

The first assumption may not be true for a number of reasons:

* The guidelines are not mandatory
* Static analysis is not perfect
* People are fallible

The larger and more distributed a project is (especially in the case of open-source software), the less likely it will be that every inch of the code-base adheres to the guidelines. The agreement that `T*` is a non-owning reference to a single object is by consensus only. It is much more reliable to use the type system to enforce correctness.

The second assumption may not be true for other reasons:

* Updating a code base to conform to the guidelines takes a lot of time
* Some code may never follow the guidelines (e.g. 3rd party libraries)

And these are just some of the reasons why it is better to be explicit than to leave people to make assumptions about your code ([I.1](https://github.com/isocpp/CppCoreGuidelines/blob/master/CppCoreGuidelines.md#Ri-explicit)).

### `T*` does not convey intent

Rule [P.1](https://github.com/isocpp/CppCoreGuidelines/blob/master/CppCoreGuidelines.md#Rp-direct) advises us to, _"Express ideas directly in code"_, noting that:

> Compilers don't read comments (or design documents) and neither do many programmers (consistently). What is expressed in code has defined semantics and can (in principle) be checked by compilers and other tools.

Rule [P.3](https://github.com/isocpp/CppCoreGuidelines/blob/master/CppCoreGuidelines.md#Rp-what) tells us to, _"Express intent"_, explaining that:

> Unless the intent of some code is stated (e.g., in names or comments), it is impossible to tell whether the code does what it is supposed to do.

Using `T*` leaves the reader of your code at a loss as to what your intent is, or what the semantics of your code are. For example,
take this function:

    void frobnicate(widget* w);

And this calling code:

    frobnicate(&wgt);

Assuming for a second that we somehow _know_ that every instance of `T*` in our code-base represents a non-owning reference to a single object (a dubious assumption to begin with), we must still ask:

* Is a copy of `w` stored for later use by `frobnicate`?
* Can we pass `frobnicate` a null pointer?

Since `T*` conveys no information other than "is a non-owning reference to a single object", we cannot know the answers to these questions without looking at the documentation. But rule [I.4](https://github.com/isocpp/CppCoreGuidelines/blob/master/CppCoreGuidelines.md#Ri-typed) tells us that "types are the simplest and best documentation", and `observer<T>` and `observer_ptr<T>` give us the answers to these questions.

    void frobnicate(observer<widget> w);

Now we _know_ that a copy of `w` is stored for later, and that we cannot pass a null pointer (the type system prevents us). If we want to be able to pass a null pointer, then we can use `observer_ptr<T>` instead. It is even clear at the call site what is going on:

    frobnicate(make_observer(wgt));

No ambiguity; no need to read documentation.

#### A slight digression

With `observer<T>` and `observer_ptr<T>` in the picture, the only role left for `T*` is as a non-owning reference to an object that is _not_ copied and stored. However, `T*` still doesn't indicate whether or not it can be null. We _could_ use `not_null<T*>` to indicate a _mandatory_ reference, but C++ already has a natural way to represent this: references.

    void frobnicate(widget& w);

This pattern is commonly used in C++ to write _non-member_ functions associated with a given class, and less commonly to pass values "out" of a function when the return value is occupied. Of course, const reference parameters are often used to avoid making an expensive or impossible copy of the argument:

    int rating(widget const& w);

Const reference parameters have the added benefit of being able to receive _temporary_ arguments:

    int len = rating(widget(42, 7, 3));

The only use of `T*` without an alternative so far is an "optional" reference to an object that is not copied and stored, as described in rule [F.60](https://github.com/isocpp/CppCoreGuidelines/blob/master/CppCoreGuidelines.md#Rf-ptr-ref):

    int rating(widget const* w);

However, for const references, we lose the ability to pass in temporaries.  We might consider using `optional<T&>`:

    int rating(optional<widget const&> w);

Unfortunately, `optional<T&>` is not currently part of the standard, as even though it was included as an auxiliary proposal to the main `optional` proposal, it was not adopted into C++17. Therefore, it seems that we currently lack an expressive way to represent "optional" reference parameters. Perhaps the GSL needs an [`optional_ref<T>`](api/optional_ref.hpp) class template?

But I digress.

### `T*` has an unsafe interface

Rule [I.4](https://github.com/isocpp/CppCoreGuidelines/blob/master/CppCoreGuidelines.md#Ri-typed) tells us to "make interfaces precisely and strongly typed". This applies to the interface of `T*` itself, not just fact that it can represent a range of conceptually different things. The interface of `T*` is inappropriate for an "observer":

* `T*` is not default initialized
* `T*` supports pointer arithmetic
* `T*` has an array subscript operator
* `T*` converts to `void*`

Using an "observer" `T*` inappropriately can result in _undefined behaviour_:

    T* obs;              // undefined behaviour (on access)
    auto x = *(obs + 1); // undefined behaviour
    auto y = obs[1];     // undefined behaviour
    delete obs;          // undefined behaviour (probably)

Conversely, `observer<T>` and `observer_ptr<T>` have none of these problems because their interface has been designed for purpose. In addition, `observer<T>` has no null state, and enforces the "not null" precondition at _compile-time_, something that neither `T*` nor `not_null<observer<T>>` can do:

    observer<T> obs1;          // error: no `observer<T>()`
    observer<T> obs2{nullptr}; // error: no `observer<T>(nullptr_t)`
    observer<T> obs3{&t};      // error: no `observer<T>(T*)`

### So what is `T*`?

In modern C++, a bare `T*` is an array iterator:

    int arr[] = { 1, 2, 3 };
    auto it = end(arr); // `decltype(it)` is `int*`

The interface of `T*` is pretty much the definition of a [random access iterator](http://en.cppreference.com/w/cpp/concept/RandomAccessIterator), so unlike its various other uses, `T*` fits this role pretty well (conversion to `void*` aside).

## Why not `optional<observer<T>>`?

It is possible to combine `observer<T>` with `optional<T>` to create `optional<observer<T>>`: an "observer" that may or may not be observing something. Thus, there is seemingly no need for `observer_ptr<T>` to exist. In principle, this approach is great, because it adheres to the "[separation of concerns](https://en.wikipedia.org/wiki/Separation_of_concerns)" and "[single responsibility](https://en.wikipedia.org/wiki/Single_responsibility_principle)" design principles, by dividing the roles of "observer" and "optional value" into separate, composable modules.

In practice, however, there is a problem with `optional<observer<T>>`: it occupies more memory than `T*` (usually twice as much) and many of its operations have overhead compared to the equivalent operations on a `T*`. This is because an implementation of `optional<T>` is typically going to look something like this:

    template <typename T>
    class optional {
        char buffer[sizeof(T)];
        bool contains_value;
    };

The overhead is associated with storing, reading and writing the `contains_value` flag. Seeing as `observer<T>` is such a simple type, and that other "single object" pointer-like abstractions (e.g. `unique_ptr<T>`) have a built-in null state, this overhead could severely hurt adoption of `observer<T>`. Given that there is no reasonable way to create a zero-overhead specialization of `optional<observer<T>>`, the only option is to create a zero-overhead counterpart to `observer<T>`: `observer_ptr<T>`.

### The history of `observer_ptr<T>`

Originally, the zero-overhead counterpart to `observer<T>` was called `optional_observer<T>`, and tried to mimic the interface of `optional<T>` by using `nullopt` to represent a _disengaged_ `optional_observer<T>`. However, since `nullopt` (or a disengaged `optional<T>`) is specified to always compare _less_ than any `T` (or engaged `optional<T>`), and the ordering of `nullptr` (or a null `T*`) compared to any non-null `T*` (using `less<T>`) is _implementation-specified_, it is _impossible_ to have a zero-overhead implementation of the `optional_observer<T>` comparison operators that are consistent with the behaviour of those of `optional<T>`.

    bool cond1 = (nullopt < optional<T>(t));
    bool cond2 = (nullopt < optional_observer<T>(t));
    assert(cond1 == cond2); // may fire (implementation-specific)

Thus, `nullopt` was replaced with `nullobs`, so that equivalent behaviour would not be assumed. However, since `optional_observer<T>` had to be constructible from `T*` to achieve zero-overhead, and was thus convertible from `nullptr_t`, `nullobs` was essentially duplicating the function of `nullptr`. Thus, `nullobs` was removed entirely, and suddenly the interface of `optional_observer<T>` looked remarkably like that of the `observer_ptr<T>` proposed for standardization, and the rest is history.