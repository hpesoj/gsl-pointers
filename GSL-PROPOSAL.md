# Proposal to add `observer` and `observer_ptr` to the GSL

## Abstract

This document makes the case for the inclusion of non-owning pointer-like types `observer<T>` and `observer_ptr<T>` in the [Guideline Support Library](https://github.com/Microsoft/GSL), and their subsequent incorporation into the [C++ Core Guidelines](https://github.com/isocpp/CppCoreGuidelines).

## Introduction

The `observer<T>` class template is a pointer-like type designed to replace `T*` wherever it is used as a non-owning "observer" of an object of type `T`.

    observer<T> o = make_observer(t);

The `observer_ptr<T>` class template is the counterpart to `observer<T>` with a _null_ state. That is, `observer_ptr<T>` is default constructible, can be constructed from `nullptr_t` and `T*`, and contextually converts to `bool`. In addition, `observer<T>` implicitly converts to `observer_ptr<T>`.

    observer_ptr<T> o = make_observer(t);

`observer<T>` and `observer_ptr<T>` are more than just non-owning references to objects of type `T`: they convey the intent to store the reference to be "observed" later on. That is, in contrast to `unique_ptr<T>` and `shared_ptr<T>`, which convey _ownership_, `observer<T>` and `observer_ptr<T>` convey _observership_.

Of course, `observer<T>` and `observer_ptr<T>` do not manage or track the lifetime of what they observer. They are designed to have no more overhead than a simple pointer, and require that the lifetimes of observers and observees are tracked further up the call stack. If lifetime tracking is required, a signals and slots implementation (e.g. [Boost.Signals2](www.boost.org/doc/libs/release/doc/html/signals2.html)) or `weak_ptr` may be used.

## Examples

`observer<T>` can be used to implement the [observer pattern](https://en.wikipedia.org/wiki/Observer_pattern):

    wgt.register_callback(make_observer(my_callback));

In this example, `register_callback` takes an `observer` parameter to indicate its intent to store a reference to the callback for later use. The caller is required to use `make_observer` and in doing so is explicitly made aware of the fact that `register_callback` is going to store a reference to `my_callback` for later use. This is a solution to the [proto-rule](https://github.com/isocpp/CppCoreGuidelines/blob/master/CppCoreGuidelines.md#S-unclassified), "Never pass a pointer down the call stack" because you never know who is going to store it. Well now you do!

Despite their names, use cases for `observer<T>` and `observer_ptr<T>` are not limited to observer pattern implementations. For example, you could construct a tree of nodes using both types:

    struct node {
        observer_ptr<node> parent;
        set<observer<node>> children;
    }; 

## Why not `T*`?

Currently, the C++ Core Guidelines state that it is appropriate to use raw pointers as observers (see rules [F.7](https://github.com/isocpp/CppCoreGuidelines/blob/master/CppCoreGuidelines.md#Rf-smart), [F.22](https://github.com/isocpp/CppCoreGuidelines/blob/master/CppCoreGuidelines.md#f22-use-t-or-ownert-to-designate-a-single-object), [F.60](https://github.com/isocpp/CppCoreGuidelines/blob/master/CppCoreGuidelines.md#f60-prefer-t-over-t-when-no-argument-is-a-valid-option) and [R.3](https://github.com/isocpp/CppCoreGuidelines/blob/master/CppCoreGuidelines.md#r3-a-raw-pointer-a-t-is-non-owning)). This section outlines why this is bad advice that violates some of the guidelines' own rules.

Both `observer<T>` and `observer_ptr<T>` are superior to `T*` in the capacity of an observer for three main reasons:

1. __Type safety__ – Neither `observer_ptr<T>` nor `observer<T>` are implicitly convertible from or to `T*`, because implicit operations should be type-safe, and `T*` can represent things other than an observer (e.g. array, string, iterator), and do not even _have_ to point to a valid object (e.g. a past-the-end iterator). Instead, observers are created from `T&` using the type-safe `make_observer` factory function, since (in a well-formed program) `T&` is _always_ a non-owning observer of a valid object.
2. __Documentation of intent__ – It is 100% clear that `observer<T>` and `observer_ptr<T>` represent mandatory and optional "observers" respectively, that will be stored and accessed later on. Conversely, it is not clear whether `T*` can or cannot be null, or whether it will be copied and stored.
3. __Interface safety__ – Both `observer<T>` and `observer_ptr<T>` have minimal interfaces that implement only operations that make sense for an observer. Conversely, `T*` support operations that do not make sense for an observer (e.g. pointer arithmetic operators, array subscript operator, conversion to `void*`). In addition, `observer<T>` enforces the "not null" precondition at _compile-time_.

### `T*` is not strongly typed

Rule [I.4](https://github.com/isocpp/CppCoreGuidelines/blob/master/CppCoreGuidelines.md#Ri-typed) tells us to "make interfaces precisely and strongly typed", explaining that types are the simplest and best documentation, have well-defined meaning, and are guaranteed to be checked at compile-time.

Rule [P.4](https://github.com/isocpp/CppCoreGuidelines/blob/master/CppCoreGuidelines.md#Rp-typesafe) says that, "ideally, a program should be statically type safe". 

An interface taking a `T*` violates these rules because `T*` is weakly typed. This is because `T*` can represent several different things:

* `T*` can be an object
* `T*` can be an array
* `T*` can be an iterator

And there are further variations of these things:

* If `T*` points to an object or an array that has been allocated on the free store, it carries no information about how or when the object or array should be _deallocated_. Deallocating an object or array incorrectly can result in _undefined behaviour_ or a _resource leak_.
* If `T*` points to an array, it carries no information about the _size_ of the array. The array may also be zero-terminated, but `T*` carries no information to indicate this either. Out of bounds array access results in _undefined behaviour_. 
* In all cases, `T*` also carries no information about whether or not it can validly be _null_. C++ emphasises performance, so it is often desirable to assume that a pointer is not null to avoid the run-time cost incurred by a conditional check. Dereferencing a null pointer results in _undefined behaviour_.

All of these things are conceptually distinct, but they all implicitly convert to each other without the compiler complaining because they are all represented by the same type, not to mention the shortcomings of the interface defined by `T*` in each case.

Of course, the C++ Core Guidelines provide alternatives to `T*` so that the safety and clarity of your code can be improved. But the guidelines state that `T*` always represents a non-owning ([R.3](https://github.com/isocpp/CppCoreGuidelines/blob/master/CppCoreGuidelines.md#r3-a-raw-pointer-a-t-is-non-owning)) reference to a single object ([F.22](https://github.com/isocpp/CppCoreGuidelines/blob/master/CppCoreGuidelines.md#f22-use-t-or-ownert-to-designate-a-single-object)). The argument is that, once all other uses of `T*` have been replaced with alternatives, all instances of `T*` will be non-owning references to single objects. This argument assumes that:

1. Everyone is following the guidelines perfectly.
2. Every part of a code-base adheres to the guidelines.

The first assumption is unlikely to be true for a number of reasons:

* The guidelines are not mandatory
* Static analysis is not perfect
* People are fallible

The larger and more distributed a project is (especially in the case of open-source software), the less likely it will be that every inch of the code-base adheres to the guidelines. The agreement that `T*` is a non-owning reference to a single object is by consensus only. It is much more reliable to use the type system to enforce correctness.

The second assumption is very unlikely

It is better to be explicit than to leave people to make assumptions about your code ([I.1](https://github.com/isocpp/CppCoreGuidelines/blob/master/CppCoreGuidelines.md#Ri-explicit)).

### `T*` does not convey intent

Rule [P.3](https://github.com/isocpp/CppCoreGuidelines/blob/master/CppCoreGuidelines.md#Rp-what) tells us to "express intent", explaining that unless the intent of some code is stated, it is impossible to tell whether the code does what it is supposed to do.

Take this function signature:

    void frobnicate(widget* w);

And this client code:

    frobnicate(&wgt);

Assuming for a second that we somehow _know_ that every instance of `T*` in our code-base represents a non-owning reference to a single object, we must still ask:

* Is a copy of `w` stored for later use by `frobnicate`?
* Can we pass `frobnicate` a null pointer?

Since `T*` conveys no information other than "is a non-owning reference to a single object", we cannot know the answers to these questions without looking at the documentation. But rule [I.4](https://github.com/isocpp/CppCoreGuidelines/blob/master/CppCoreGuidelines.md#Ri-typed) tells us that "types are the simplest and best documentation", and `observer<T>` and `observer_ptr<T>` give us the answers to these questions.

    void frobnicate(observer<widget> w);

Now we _know_ that a copy of `w` is stored for later, and that we cannot pass a null pointer (the type system prevents us). If we want to be able to pass a null pointer, then we can use `observer_ptr<T>` instead. It is even clear at the call site what is going on:

    frobnicate(make_observer(wgt));

No ambiguity; no need to read documentation.

So what role is left for `T*`? If `w` is not copied and stored, then it is inappropriate to use `observer<T>` or `observer_ptr<T>`. Therefore, the only case where we might consider using `T*` is passing a non-owning reference to an object that is not copied and stored. However, `T*` still doesn't indicate whether or not it can be null. We _could_ use `not_null<T*>`, but there is a much more natural way to pass non-owning  _mandatory_ references to objects: references.

    void frobnicate(widget& w);

This pattern is commonly used in C++ to write _non-member_ functions associated with a given class, and less commonly to pass values "out" of a function when the return value is occupied. Of course, const reference parameters are often used to avoid making an expensive or impossible copy of the argument:

    int rating(widget const& w);

They also have the added benefit of being able to receive _temporary_ arguments:

    auto len = rating(widget(42, 7, 3));

The only use case unaccounted for so far is an "optional" reference to an object that is not copied and stored, as described in rule [F.60](https://github.com/isocpp/CppCoreGuidelines/blob/master/CppCoreGuidelines.md#Rf-ptr-ref):

    int rating(widget const* w);

However, for const references, we lose the ability to pass in temporaries.  We might consider using `optional<T&>`:

    int rating(optional<widget const&> w);

Unfortunately, `optional<T&>` is not currently part of the standard, as even though it was included as an auxiliary proposal to the main `optional` proposal, it was not adopted into C++17. Therefore, it seems that we currently lack an expressive way to represent "optional" reference parameters. Perhaps the GSL needs an `optional_ref<T>` class template?

    int rating(optional_ref<widget const> w);

But I digress.

### `T*` has an unsafe interface

Rule [I.4](https://github.com/isocpp/CppCoreGuidelines/blob/master/CppCoreGuidelines.md#Ri-typed) tells us to "make interfaces precisely and strongly typed". This applies to the interface of `T*` itself, not just fact that it can represent a range of conceptually different things. The interface of `T*` is inappropriate for an "observer":

* `T*` is not zero initialized
* `T*` supports pointer arithmetic
* `T*` has an array subscript operator
* `T*` converts to `void*`

Using an "observer" `T*` inappropriately can result in _undefined behaviour_:

    T* obs;              // undefined behaviour (later)
    auto x = *(obs + 1); // undefined behaviour
    auto y = obs[1];     // undefined behaviour
    delete obs;          // undefined behaviour (probably)

Conversely, `observer<T>` and `observer_ptr<T>` have none of these problems because their interface has been designed for their purpose. In addition, `observer<T>` has no null state, and enforces the "not null" precondition at _compile-time_:

    observer<T> obs;          // error: no `observer<T>()`
    observer<T> obs{nullptr}; // error: no `observer<T>(nullptr_t)`
    observer<T> obs{&t};      // error: no `observer<T>(T*)`

### So what is `T*`?

In modern C++, a bare `T*` is an array iterator:

    int arr[] = { 1, 2, 3 };
    auto it = end(arr); // `decltype(it)` is `int*`

The interface of `T*` is pretty much the definition of a [random access iterator](http://en.cppreference.com/w/cpp/concept/RandomAccessIterator), so unlike its various other uses, `T*` fits this role pretty well (conversion to `void*` aside). Of course, there are various superior alternatives to C-style arrays ([ES.27](https://github.com/isocpp/CppCoreGuidelines/blob/master/CppCoreGuidelines.md#Res-stack)), and `span` is recommended instead of direct use of iterators, so `T*` probably won't be seen in this capacity much in modern C++ code.