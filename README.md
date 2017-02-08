# Pointers and the C++ Core Guidelines

_Joseph Thomson (<joseph.thomson@gmail.com>)_<br>
8 February 2017

## Introduction

This document outlines why the the [C++ Core Guidelines](https://github.com/isocpp/CppCoreGuidelines)' current recommendations regarding the use of pointers in modern C++ code are not ideal. It goes on to make the case for the addition of two types, [`observer<T>`](api/gsl/observer.hpp), and [`optional_ref<T>`](api/gsl/optional_ref.hpp), to the [Guideline Support Library](https://github.com/Microsoft/GSL). Finally, it discusses the role of `owner<T>` and `not_null<T>` in the validation of code by static analysis tools, and suggests a refined approach to pointer annotation.

## The problem

### Type-safety

Pointer types define a large range of operations. However, many of these operations have well-defined behaviour only in specific circumstances. For example (given a pointer, `p`, and an integral, `n`):

* The expression `p + n` is meaningless (for `n != 0`) if `p` points to a single object.
* The expression `*p` has undefined behaviour if `p` is a null pointer or a past-the-end iterator.
* The expression `delete p` has undefined behaviour if `p` points to an object not allocated with `new`.

The problem is that `T*` is _weakly typed_ as it can represent many different things, all of which share a single, multi-purpose interface. Rule [I.4](https://github.com/isocpp/CppCoreGuidelines/blob/master/CppCoreGuidelines.md#Ri-typed) tells us to, _"Make interfaces precisely and strongly typed"_, explaining that:

> Types are the simplest and best documentation, have well-defined meaning, and are guaranteed to be checked at compile-time.

In addition, rule [P.1](https://github.com/isocpp/CppCoreGuidelines/blob/master/CppCoreGuidelines.md#Rp-direct) advises us to, _"Express ideas directly in code"_, noting that:

> Compilers don't read comments (or design documents) and neither do many programmers (consistently). What is expressed in code has defined semantics and can (in principle) be checked by compilers and other tools.

By using `T*`, we make poor use of the type system, providing insufficient information to both the programmer and the compiler about our intent. `T*` has overly broad semantics, and we would benefit from replacement of `T*` with _strongly typed_ alternatives. The guidelines currently recommend various such types—`unique_ptr`, `shared_ptr`, `array`, `stack_array`—but there are still places where the guidelines recommend the use of `T*`. Rule [F.22](https://github.com/isocpp/CppCoreGuidelines/blob/master/CppCoreGuidelines.md#f22-use-t-or-ownert-to-designate-a-single-object) advises us to, _"Use `T*` or `owner<T*>` to designate a single object"_, giving the reasoning:

> Readability: it makes the meaning of a plain pointer clear. Enables significant tool support.

Designating a single purpose for `T*` does indeed make the meaning of a `T*` clear and enable tool support, but no reason is given why this particular use would not benefit from a strongly typed replacement. Rule [Bounds.1](https://github.com/isocpp/CppCoreGuidelines/blob/master/CppCoreGuidelines.md#Pro-bounds-arithmetic) instructs, _"Don't use pointer arithmetic. Use `span` instead"_, explaining:

> Pointers should only refer to single objects, and pointer arithmetic is fragile and easy to get wrong. `span` is a bounds-checked, safe type for accessing arrays of data.

It appears that the guidelines are reasoning that, since `span` is provided to replace pointers when used as iterators, the only use left for `T*` is as a reference to single objects, so by process of elimination this must be _the_ single appropriate use for `T*` in modern C++ code. However, pointer arithmetic operations are _always_ wrong if `T*` points to a single object, and dereferencing a null pointer gives undefined behaviour, yet the guidelines recommend its use. Of course, static analysis tools _could_ flag "inappropriate" uses of `T*`, but this is still not type-safe, relies on the presence of static analysis tools, and doesn't make this particular guideline any less arbitrary.

Rather than somewhat arbitrarily declaring one of the many uses of `T*` to be the sole "approved" use in modern C++ code, and trying to compensate for the lack of type-safety using static analysis, why not provide guidelines on how to replace _all_ instances of `T*` with modern, _type-safe_ abstractions? In theory, the only place that `T*` will even be used is in when implementing those abstractions. This is a clear and consistent approach that adheres to the guidelines' own recommendations to make strongly typed code and to document intent in-code.

### Documentation of intent

If type-safety is not a compelling enough reason to replace `T*` in all high-level code, consider that the guidelines still recommend the use of `T*` for two _conceptually distinct_ purposes.

The first use of `T*` is described in rule [F.60](https://github.com/isocpp/CppCoreGuidelines/blob/master/CppCoreGuidelines.md#Rf-ptr-ref), which tells us to, _"Prefer `T*` over `T&` when "no argument" is a valid option"_, explaining:

> A pointer (`T*`) can be a `nullptr` and a reference (`T&`) cannot, there is no valid "null reference". Sometimes having `nullptr` as an alternative to indicated "no object" is useful, but if it is not, a reference is notationally simpler and might yield better code.

However, there is a soon to be standardized way to represent the concept of "no object" in C++: `optional<T>`. The advantage of `optional<T&>` over `T*` would be that `optional<T const&>` parameters can accept temporary arguments, just like `T const&`. Unfortunately, although `optional<T&>` was included as an auxiliary proposal to the main `optional<T>` proposal, it doesn't look like it will be accepted into the standard. However, `optional<T&>` _is_ in theory a much better way to represent optional reference parameters than `T*`.

The second use of `T*` is not explicitly described by the guidelines, but can be identified by considering how best to _store_ a non-owning reference. If `T&` is the appropriate way to represent a non-optional reference parameter, we might consider using a `T&` data member to _store_ a reference:

    class foo {
    public:
        explicit foo(bar const& b) : b(b) {}
        …
    private:
        bar const& b;
    };

Unfortunately, `T&` data members make their containing class non-copy assignable by default. In addition, they cannot be stored in STL containers. Instead, we could use `T*`, but our reference is non-optional, so we had better use `not_null<T*>`:

    class foo {
    public:
        explicit foo(bar const& b) : b(&b) {}
        …
    private:
        not_null<bar const*> b;
    };

However, there are still a number of problems with this approach. Firstly, it is not generally expected that a function taking a `T&` parameter will retain a pointer to its argument. The intent to retain is unclear from looking at either the function signature _or_ the calling code:

    foo f{b}; // looks like "pass by value"

Rule [F.15](https://github.com/isocpp/CppCoreGuidelines/blob/master/CppCoreGuidelines.md#Rf-conventional) tells us to, _"Prefer simple and conventional ways of passing information"_, explaining:

> Using "unusual and clever" techniques causes surprises, slows understanding by other programmers, and encourages bugs.

Storing a pointer to a `T const&` parameter could be considered "unusual" and is arguably an example of a violation of this rule, as conventionally, a `T const&` parameter is behaviourally similar to a `T` parameter (a copy of the argument may or may not be retained). Furthermore, it is possible to pass temporaries to `T const&` parameters, which in this case will inevitably result in dangling pointers.

    foo f{bar()}; // `f.b` is left dangling

We could instead take a `not_null<T*>` parameter, but then we would lose the compile-time enforcement of the "not null" condition (this is also a disadvantage of using `not_null<T*>` as a data member, though in this case the member is private, so opportunities for bugs to arise are encapsulated within the class). In addition, the calling code is indistinguishable from that of a `T*` "optional reference" parameter, and it is _still_ likely to be surprising that a copy of the pointer is retained. If a function is going to store a copy of a reference or pointer parameter, the intent to do so should be made _explicitly clear_ in both the function signature _and_ at the call site. Currently, there is no conventional way to do this in C++.

## The solution

There are two uses of `T*` for which the guidelines do not give strongly typed alternatives:

1. A retained, non-owning reference to an object
2. An "optional reference" parameter

Proposed here are two class templates, `observer<T>` and `optional_ref<T>`, that can be used in place of `T*` in these two situations.

### `observer<T>`

The [`observer<T>`](api/gsl/observer.hpp) class template is a pointer-like type designed to replace `T*` wherever it is used as a non-owning "observer" of an object of type `T`.

    class foo {
    public:
        explicit foo(observer<bar const> b) : b(b) {}
        …
    private:
        observer<bar const> b;
    };

Because `observer<T>` is only _explicitly_ constructible from `T&`, the intention to retain a copy of the reference has to be explicitly documented at the call site, usually via the `make_observer` factory function:

    foo f{make_observer(b)}; 

In addition, `observer<T>` disables construction from `T&&`, so it cannot be constructed from temporary objects.

    foo f{make_observer{bar()}}; // error: `make_observer(T&&)` is deleted

As a zero-overhead replacement for `T*`, `observer<T>` does not manage or track the lifetime of what it observes. If automatic lifetime tracking is required, alternative approaches such as a signals and slots implementation (e.g. [Boost.Signals2](www.boost.org/doc/libs/release/doc/html/signals2.html)) or `weak_ptr` may be used.

An `observer<T>` has no "null" state and must therefore always point to an object. Thus, it enforces the "not null" condition at compile-time, just like `T&`. If the ability to represent "no object" is required, `observer<T>` can be combined with `optional<T>`:

    class foo {
    public:
        foo() = default;
        explicit foo(observer<bar const> b) : b(b) {}
        …
    private:
        optional<observer<bar const>> b;
    };

Or alternatively:

    class foo {
    public:
        explicit foo(optional<observer<bar const>> b = nullopt) : b(b) {}
        …
    private:
        optional<observer<bar const>> b;
    };

#### Zero-overhead optimizations

A naive implementation of `optional<observer<T>>` will not have zero-overhead (most notably, it usually occupies twice as much memory as `T*`). However, the [as-if rule](http://en.cppreference.com/w/cpp/language/as_if) _should_ allow a zero-overhead implementation in practice if an `optional<observer<T>>` specialization is given "back-door" access to the pointer member internal to `observer<T>`, the unused null pointer state of which it can use to represent its _disengaged_ state.

### `optional_ref<T>`

The [`optional_ref<T>`](api/gsl/optional_ref.hpp) class template is an "optional" reference-like type designed to allow the implementation of "optional reference" parameters.

    void frobnicate_default();
    void frobnicate_widget(widget const& w);

    void frobnicate(optional_ref<widget const> ow) {
        if (ow) {
            frobnicate_widget(*ow);
        } else {
            frobnicate_default();
        }
    }

Since `T&` converts implicitly to `optional_ref<T>`, it is possible to pass temporaries to `optional_ref<T const>` parameters just as with `T const&` parameters:

    frobnicate(widget());

An `optional_ref<T>` is essentially an alternative implementation of `optional<T&>`. The design differs somewhat from the `optional<T&>` described by the auxiliary proposal of [N3527](http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2013/n3527.html).

#### Copy assignment

The `optional<T&>` auxiliary proposal discussed how the semantics for copy assignment proved controversial. They ultimately chose _reference_ copy assignment semantics somewhat arbitrarily, copying the behaviour of `reference_wrapper` and `boost::optional`. The proposal mentions that most people insisted that `optional<T&>` be copy assignable, but no rationale was given. We speculate that this is because `optional<T>` is viewed as inheriting the operations supported by `T`, and `T&` is copy assignable (although copy assignment for `T&` has _value_ semantics, not reference semantics).

We believe it is more helpful to think of `optional<T>` as a _container_ of `T`; after all, `optional<T>` doesn't inherit the rest of the interface of `T`, nor would it be correct to try to do so. Thus, we decided to disable copy assignment for `optional_ref<T>`, as classes that contain `T&` are _not_ copy assignable. The desire to implement value semantic copy assignment may also be in part due to the misconception that the inability to rebind `T&` is a "flaw" in its design. On the contrary, the inability to rebind `T&` is a feature that is _intrinsic_ to its nature. The role of the `optional<T&>` suggested in the auxiliary proposal can be played by `optional<reference_wrapper<T>>`.

#### Construction from `T&&`

Allowing construction from `T&&` is essential to allow passing of temporary arguments to `optional_ref<T>` parameters. The `optional` proposal decided to disable construction from `T&&` to prevent the accidental formation of dangling references by programmers who mistakenly expect `optional<T const&>` to extend the lifetime of temporaries like `T const&` does. However, we believe this is a price worth paying to enable temporaries to be passed as `optional_ref<T const>` parameters, especially given that `string_view`, a reference-like type with similar semantics, make the same compromise.

### `optional<T>`

Currently, nowhere in the guidelines is `optional<T>` mentioned. This is understandable, since C++17 is still a work-in-progress. However, even when this new version of the standard is released, it will be some time before everyone following the guidelines has access to a production-ready implementation. Given the fundamental role that `optional<T>` plays in expressing meaning and updating old C++ code to be safer and more reliable, we suggest adding an implementation `optional<T>` to the GSL. Note that we recommend _also_ adding `optional_ref<T>`, as opposed to implementing `optional<T&>`, since any implementation of `optional<T>` should ideally be standard-conforming. If and when `optional<T&>` is standardized, the guidelines can be updated, and instances of `optional_ref<T>` can easily be find-and-replaced (providing `optional<T&>` has roughly the same semantics).

## Pointer annotations

Introducing `observer<T>` and `optional_ref<T>` to the GSL and guidelines would not remove the need for annotations such as `owner<T>` and `not_null<T>`. Annotations are necessary to help static analysis tools verify the integrity of code that cannot use high-level types for any of a variety of reasons (most obviously, the implementation of the types themselves). However, there are a number of things to be said about the current approach.

Pointer types, as explained in this document, are multi-purpose, and thus support far to broad a range of operations for any one use case:

* Arithmetic operations make sense only for pointers to elements of an array
* Assigning and checking for null makes sense only for pointers that can be null
* Calling `delete` makes sense only for pointers to heap-allocated non-array objects
* Calling `delete[]` makes sense only for pointers to heap-allocated array objects

We suggest that a _bare_ `T*` (i.e. one without any annotations) _should_ only represent a non-owning pointer to a single object (say what??); this use just has no place in _high-level_, _modern_ C++ code. However, `T*` should also be implicitly "not null". This allows individual "features" of pointers to be _enabled_ using annotations, in the vein of `owner<T>`, rather than the approach taken by `not_null<T>`, which is to _disable_ a particular feature. Therefore, we suggest supporting the following annotations, one to enable each of the "features" listed above:

* `iterator<T>` enables pointer arithmetic operations
* `nullable<T>` enables the null pointer state
* `owner<T>` enables use of `delete`
* `array_owner<T>` enables use of `delete[]`

An example of correct use of these annotations is:

                   T*   p1 = &t;
    nullable<owner<T*>> p2 = new (nothrow) T;
             owner<T*>  p3 = new T;
       array_owner<T*>  p4 = new T[n];
          iterator<T*>  p5 = p3;

In particular, it is important to distinguish between owners of objects and owners of arrays, something the guidelines currently do not account for. This approach will enforce the strictest use of `T*` by default, allowing certain "dangerous" features to be enabled one-by-one. Static analysis tools can warn wherever a "feature" is used without its corresponding annotation. For example:

    p1++;         // warning: pointer arithmetic without `iterator`
    T t2 = *p2;   // warning: dereferencing `nullable` without null check
    if (p3) { … } // warning: checking for null without `nullable`
    delete p4;    // warning: calling `delete` without `owner`
    p5 = nullptr; // warning: setting to null without `nullable`

Of course, such warnings are likely to be ubiquitous in old C++ code, but there is really no way around this if your goal to make your code safer, more explicit and free of bugs. Static analysis tools can always provide the ability to disable particular categories of warning, or only show warnings for particular files or sections of code, to make updating old code more manageable.

### `nullable<T>` vs `not_null<T>`

The `nullable<T>` annotation, as a mere template type alias, lacks the ability to enforce the "not null" condition at run-time like `not_null<T>`. This may seem like a loss of functionality, but considering that debug builds are likely to be able to catch attempts to dereference null pointers at run-time, and that run-time checks will probably be turned off for release builds, along with the fact that use of `T&`, `observer<T>` and `span<T>` allows the "not null" condition to be enforced at _compile-time_, `not_null<T>` may actually provide little additional value.

One _could_ argue that `not_null<T>` could be used in situations where switching to a higher-level abstraction would break too much client code. However, `not_null<T>` explicitly disables pointer arithmetic, which means that it already breaks code where `T*` is used as an iterator. In fact, our assessment is that `not_null<T>` is actually a hybrid between an annotation and a high-level type. It is simultaneously trying to facilitate the job of static analysis tools _and_ itself perform safety checks at run-time (the compile-time safety of `not_null<T>` has also been discussed). In addition, the question arises, how would one annotate `not_null<T>` to verify that _its_ implementation is correct?

Given that the introduction of `observer<T>` and `optional_ref<T>` would all but replace use of `not_null<T*>` in high-level code, and that `not_null<T*>` seems to have an inherent incompatibility with smart pointer types, it seems reasonable to replace it with the `nullable<T>` annotation. It would not be necessary to remove `not_null<T>` from the GSL immediately, if there is concern that doing so would break a lot of existing code.

## In summary

The aim of the C++ Core Guidelines is to facilitate writing of modern C++ code. At the core of modern C++ is _type-safety_, and pointers are _not_ type-safe. Rather than inventing arbitrary rules about how `T*` should be used in modern, high-level C++ code, we should provide _type-safe_, modern abstractions to replace _all_ of the old, unsafe uses of `T*`. In addition, a full range of type alias pointer annotations should be provided to allow the features of pointers to be enabled one-by-one wherever it is not possible to replace `T*` with higher-level abstractions. This will allow effective validation of low-level and legacy code using static analysis, including standard library and GSL implementations.