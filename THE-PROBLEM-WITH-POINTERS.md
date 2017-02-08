# The problem with pointers

_Joseph Thomson (<joseph.thomson@gmail.com>)_<br>
8 February 2017

## Introduction

This document outlines why the the [C++ Core Guidelines](https://github.com/isocpp/CppCoreGuidelines)' current recommendations regarding the use of pointers in modern C++ code are not ideal. It goes on to make the case for the addition of a pointer-like "observer" type, [`observer<T>`](api/gsl/observer.hpp), and an "optional" reference-like type, [`optional_ref<T>`](api/gsl/optional_ref.hpp), to the [Guideline Support Library](https://github.com/Microsoft/GSL). Finally, it discusses the role of `owner<T>` and `not_null<T>` in the validation of code by static analysis tools, and suggests a refined approach to pointer annotation.

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

By using `T*`, we make poor use of the type system, providing insufficient information to both the programmer and the compiler about our intent. `T*` has overly broad semantics, and we would benefit from replacement of `T*` with _strongly typed_ alternatives. The guidelines currently recommend various such types—`unique_ptr`, `shared_ptr`, `array`, `stack_array`—but there is still one use of `T*` that has no replacement. Rule [F.22](https://github.com/isocpp/CppCoreGuidelines/blob/master/CppCoreGuidelines.md#f22-use-t-or-ownert-to-designate-a-single-object) advises us to, _"Use `T*` or `owner<T*>` to designate a single object"_, giving the reasoning:

> Readability: it makes the meaning of a plain pointer clear. Enables significant tool support.

Designating a single purpose for `T*` does indeed make the meaning of a `T*` clear and enable tool support, but no reason is given why this particular purpose should be designated. Rule [Bounds.1](https://github.com/isocpp/CppCoreGuidelines/blob/master/CppCoreGuidelines.md#Pro-bounds-arithmetic) instructs, _"Don't use pointer arithmetic. Use `span` instead"_, explaining:

> Pointers should only refer to single objects, and pointer arithmetic is fragile and easy to get wrong. `span` is a bounds-checked, safe type for accessing arrays of data.

It appears that the guidelines are reasoning that, since `span` is provided to replace pointers when used as iterators, the only use left for `T*` is as a reference to single objects, so by process of elimination this must be _the_ single appropriate use for `T*` in modern C++ code. However, pointer arithmetic operations are _always_ wrong if `T*` points to a single object, and dereferencing a null pointer gives undefined behaviour, yet the guidelines recommend its use. Of course, static analysis tools _could_ flag "inappropriate" uses of `T*`, but this doesn't make this particular guideline any less arbitrary. Rule [R.2](https://github.com/isocpp/CppCoreGuidelines/blob/master/CppCoreGuidelines.md#Rr-use-ptr) notes that flagging such uses "would generate a huge number of false positives if applied to an older code base".

Rather than somewhat arbitrarily declaring one of the many uses of `T*` to be the sole "approved" use in modern C++ code, and trying to compensate for the lack of type-safety using static analysis, why not provide guidelines on how to replace _all_ instances of `T*` with modern, _type-safe_ abstractions? In theory, the only place that `T*` will even be used is in when implementing those abstractions. This is a clear and consistent approach that adheres to the guidelines' own recommendations to make strongly typed code and to document intent in-code.

### Documentation of intent

If type-safety is not a compelling enough reason to replace `T*` in all high-level code, consider that the guidelines still recommend the use of `T*` for two _conceptually distinct_ purposes.

The first use of `T*` is described in rule [F.60](https://github.com/isocpp/CppCoreGuidelines/blob/master/CppCoreGuidelines.md#Rf-ptr-ref), which tells us to, _"Prefer `T*` over `T&` when "no argument" is a valid option"_, explaining:

> A pointer (`T*`) can be a `nullptr` and a reference (`T&`) cannot, there is no valid "null reference". Sometimes having `nullptr` as an alternative to indicated "no object" is useful, but if it is not, a reference is notationally simpler and might yield better code.

However, there is soon to be a standardized way to represent the concept of "no object" in C++: `optional<T>`. The advantage of `optional<T&>` over `T*` would be that `optional<T const&>` parameters can accept temporary arguments, just like `T const&`. Unfortunately, although `optional<T&>` was included as an auxiliary proposal to the main `optional<T>` proposal, it doesn't look like it will be accepted into the standard. However, `optional<T&>` _is_ in theory the correct way to represent optional reference parameters, not `T*`.

The second use of `T*` is not explicitly described by the guidelines, but can be identified by considering how best to _store_ a non-owning reference. If `T&` is the appropriate way to represent a non-optional reference parameter, we might consider using it to store the reference as well:

    class foo {
    public:
        explicit foo(bar const& b) : b(b) {}
        …
    private:
        bar const& b;
    };

Unfortunately, `T&` data members make the containing class non-copy assignable by default. In addition, they cannot be stored in containers. Instead we might consider using `T*`, but our reference is non-optional, so we had better use `not_null<T*>`:

    class foo {
    public:
        explicit foo(bar const& b) : b(&b) {}
        …
    private:
        not_null<bar const*> b;
    };

However, there are still a number of problems with this approach. Firstly, it is not generally expected that a function taking a `T&` parameter will retain a pointer to its argument. The intent to retain is unclear from looking at either the function signature _or_ the calling code:

    foo f{b};

Rule [F.15](https://github.com/isocpp/CppCoreGuidelines/blob/master/CppCoreGuidelines.md#Rf-conventional) tells us to, _"Prefer simple and conventional ways of passing information"_, explaining:

> Using "unusual and clever" techniques causes surprises, slows understanding by other programmers, and encourages bugs.

This is a very good example of a violation of this rule. Furthermore, it is possible to pass temporaries to `T const&` parameters, which will inevitably result in dangling pointers.

    foo f{bar()}; // `f.b` is left dangling

We could instead take a `not_null<T*>` parameter, but then we would lose the compile-time enforcement of the "not null" condition (this is also a disadvantage of using `not_null<T*>` as a data member, though in this case any associated dangers are confined to the class implementation). In addition, the calling code is indistinguishable from that of an "optional" reference parameter, and it is _still_ likely to be surprising that a copy of the pointer is retained. If a function is going to store a copy of a reference or pointer parameter, the intent to do so must be made _explicitly clear_ in both the function signature _and_ at the call site. Currently, there is no standard or recommended way to do this in C++.

## The solution

There are two uses of `T*` for which the guidelines do not give strongly typed alternatives:

1. A retained, non-owning reference to an object
2. An "optional reference" parameter

Proposed here are two class templates, `observer<T>` and `optional_ref<T>`, that can be used in place of `T*` in these two situations. They solve all of the problems with using `T*` that have been listed so far.

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

An `observer<T>` has no "null" state and therefore must always point to an object. Thus, it enforces a "not null" condition at compile-time, just like `T&`. If the ability to represent "no object" is required, `observer<T>` can be combined with `optional<T>`:

    class foo {
    public:
        foo() = default;
        explicit foo(observer<bar const> b) : b(b) {}
        …
    private:
        optional<observer<bar const>> b;
    };

#### Zero-overhead optimizations

A naive implementation of `optional<observer<T>>` will not have zero-overhead (most notably, it usually occupies twice as much memory as `T*`). However, the [as-if rule](http://en.cppreference.com/w/cpp/language/as_if) _should_ allow a zero-overhead implementation in practice if an `optional<observer<T>>` specialization is given "back-door" access to the pointer member internal to `observer<T>`, the unused null pointer state of which it can use to represent its _disengaged_ state.

### `optional_ref<T>`

The [`optional_ref<T>`](api/gsl/optional_ref.hpp) class template is an "optional" reference-like type designed to allow the implementation of "optional reference" parameters.

    void frobnicate_default();
    void frobnicate_widget(widget& w);

    void frobnicate(optional_ref<widget> ow) {
        if (ow) {
            frobnicate_widget(*ow);
        } else {
            frobnicate_default();
        }
    }

An `optional_ref<T>` is essentially an alternative implementation of `optional<T&>`. The design differs somewhat from the `optional<T&>` described by the auxiliary proposal of N3527:

* Construction from `T&&` is not disabled—This is essential to allow passing of temporary arguments to `optional_ref<T>` parameters, a key use case missed by the proposal. It is worth noting that `string_view`, a type with similar reference-like semantics intended to be used to pass non-owning "views" of string objects into functions, does not disable construction from `string&&` either.
* Copy assignment is disabled—The auxiliary proposal discussed how the semantics for copy assignment proved controversial. They ultimately chose _reference_ copy semantics somewhat arbitrarily, following the behaviour of `reference_wrapper` and `boost::optional`. `optional_ref<T>` avoids the need to make this arbitrary decision by disabling copy assignment. The proposal _does_ mention that most people insisted that `optional` (`optional<T&>`?) be copy assignable, but no rationale was given. We reason that `optional<T&>` should behave as though it _contains_ a `T&` data member, just as `optional<T>` behaves as though it contains a `T` data member. The aversion to disabling copy assignment may be due to the fact that many consider the inability to rebind `T&` a "flaw" in its design. On the contrary, the inability to rebind `T&` is a feature that is _fundamental_ to its nature. By extension, the inability to rebind `optional<T&>`, and thus `optional_ref<T>`, is perfectly natural.  
* `swap` and `emplace` are omitted—This is for the same reason copy assignment is disabled.
* Emplacement constructors are omitted—These seem to have no function since no objects are actually constructed in-place.
* Construction from compatible `optional_ref<T>` types is supported—It isn't clear why this was omitted from the proposal.

### `optional<T>`

Currently, nowhere in the guidelines is `optional<T>` mentioned. This is understandable, since C++17 is still a work-in-progress. However, even when this new version of the standard is released, it will be some time before everyone following the guidelines has access to a production-ready implementation. Given the fundamental role that `optional<T>` plays in expressing meaning and updating old C++ code to be safer and more reliable, we suggest adding an implementation `optional<T>` to the GSL. Note that we recommend _also_ adding `optional_ref<T>`, as opposed to implementing `optional<T&>`, since any implementation of `optional<T>` should ideally be standard-conforming. If and when `optional<T&>` is standardized, the guidelines can be updated, and instances of `optional_ref<T>` can easily be find-and-replaced (providing `optional<T&>` has roughly the same semantics).

## Pointer annotations

Introducing `observer<T>` and `optional_ref<T>` to the GSL and guidelines would not remove the need for annotations such as `owner<T>` and `not_null<T>`. Annotations are necessary to help static analysis tools verify the integrity of code that cannot use high-level types for any of a variety of reasons (most obviously, the implementation of the types themselves). However, there are a number of things to be said about the current approach.

Pointer types, as explained in this document, are multi-purpose, and thus support far to broad a range of operations for any one use case:

* Arithmetic operations make sense only for pointers to elements of an array
* Assigning and checking for null makes sense only for pointers that can be null
* Calling `delete` makes sense only for pointers to heap-allocated non-array objects
* Calling `delete[]` makes sense only for pointers to heap-allocated array objects

We suggest that a _bare_ `T*` (i.e. one without any annotations) _should_ only represent a non-owning pointer to a single object (say what??); this use just has no place in _high-level_, _modern_ C++ code. However, `T*` should also be implicitly "not null". This allows individual "features" of pointers to be _enabled_ using annotations, in the vein of `owner<T>`, rather than the approach taken by `not_null<T>`, which is to _disable_ a particular feature. Therefore, we recommend support for the following annotations, one to enable each of the "features" listed above:

* `iterator<T>` enables pointer arithmetic operations
* `nullable<T>` enables the null pointer state
* `owner<T>` enables use of `delete`
* `array_owner<T>` enables use of `delete[]`

In particular, it is important to distinguish between owners of objects and owners of arrays, something the guidelines currently do not account for. This approach will enforce the strictest use of `T*` by default, allowing certain "dangerous" features to be enabled one-by-one. Static analysis tools can warn wherever a "feature" is used without its corresponding annotation. For example:

    T* p = &t;
    p++;         // warning: pointer arithmetic requires `iterator`
    p = nullptr; // warning: assigning `nullptr` requires `nullable`
    delete p;    // warning: calling `delete` requires `owner`

Of course, such warnings are likely to be ubiquitous in old C++ code, but there is really no way around this if your goal to make your code safer, more explicit and free of bugs. Static analysis tools can provide the ability to disable particular categories of warning, or only show warnings for particular files or sections of code. This should make updating old code more manageable. 

### `nullable<T>` vs `not_null<T>`

The `nullable<T>` annotation, as a simple template type alias, obviously lacks the ability to check the "not null" precondition at run-time like `not_null<T>`. This may seem like a loss of functionality, but considering that debug builds are likely to be able to catch attempts to dereference null pointers at run-time, and that run-time checks will probably be turned off for release builds, along with the fact that use of `T&`, `observer<T>` and `span<T>` allows the "not null" precondition to be enforced at _compile-time_, `not_null<T>` may actually provide little of value.

One _could_ argue that `not_null<T>` could be used in situations where switching to a higher-level abstraction would break too much client code. However, `not_null<T>` explicitly disables pointer arithmetic, which means that it already breaks code where `T*` is used as an iterator. In fact, our assessment is that `not_null<T>` is actually a hybrid between an annotation and a high-level type. It is simultaneously trying to facilitate the job of static analysis tools _and_ itself perform safety checks at run-time (the compile-time safety of `not_null<T>` has also been discussed). In addition, the question arises, how would one annotate `not_null<T>` to verify that _its_ implementation is correct?

It would not be necessary to remove `not_null<T>` from the GSL immediately, if there is concern that this would break a lot of existing code.

## In summary

The aim of the C++ Core Guidelines is to facilitate writing of modern C++ code. At the core of modern C++ is _type-safety_, and pointers are _not_ type-safe. Rather than inventing arbitrary rules about how `T*` should be used in modern, high-level C++ code, we should provide _type-safe_, modern abstractions to replace _all_ of the old, unsafe uses of `T*`. In addition, a full range of type alias pointer annotations should be provided to allow the features of pointers to be enabled one-by-one wherever it is not possible to replace `T*` with higher-level abstractions. This will allow effective validation of low-level and legacy code, including standard library and GSL implementations, using static analysis.