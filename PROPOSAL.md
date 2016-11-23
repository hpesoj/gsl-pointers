# A proposal to add non-owning indirection class templates `indirect` and `optional_indirect`

_Joseph Thomson \<joseph.thomson@gmail.com\>_

## Introduction

`indirect<T>` and `optional_indirect<T>` are high-level indirection types that reference objects of type `T` without implying ownership. An `indirect<T>` is an indirect reference to an object of type `T`, while an `optional_indirect<T>` is an _optional_ indirect reference to an object of type `T`. Both `indirect` and `optional_indirect` have reference-like construction semantics and pointer-like assignment and indirection semantics.

## Table of contents

* [Motivation and scope](#motivation)
* [Impact on the standard](#impact)
* [Design decisions](#design)
  * [Construction from `T&`](#design/construction-lvalue-ref)
  * [Conversion to `T&`](#design/conversion-lvalue-ref)
  * [Construction from `T*`](#design/construction-ptr)
  * [Conversion to `T*`](#design/conversion-ptr)
  * [Construction from `T&&`](#design/construction-rvalue-ref)
  * [Assignment operators and the `{}` idiom](#design/assignment)
  * [Optional API functions](#design/optional)
  * [Comparison operations](#design/comparison)
  * [Move behaviour](#design/move)
  * [Relationships and immutability](#design/immutability)
  * [Const-propagation](#design/const-propagation)
  * [The `make` free functions](#design/make-functions)
  * [The `cast` free functions](#design/cast-functions)
  * [The `get_pointer` free functions](#design/get_pointer-functions)
  * [Compatibility with `propagate_const`](#design/propagate-const)
  * [Use of inheritance by delegation ("`operator.` overloading")](#design/operator-dot)
  * [Why not `T*`?](#design/why-not-ptr)
  * [Why not `T&`?](#design/why-not-ref)
  * [Why not `optional<T&>`?](#design/why-not-optional-ref)
  * [Why not `optional<indirect<T>>`?](#design/why-not-optional-indirect)
  * [Why not `observer_ptr<T>`?](#design/why-not-observer_ptr)
  * [Why not `reference_wrapper<T>`?](#design/why-not-reference_wrapper)
  * [Why not `not_null<T*>`?](#design/why-not-not_null)
  * [Naming](#design/naming)
* [Technical specifications](#technincal)
* [Acknowledgements](#acknoledgements)
* [References](#references)

## <a name="motivation"></a>Motivation and scope

Modern C++ [guidelines](https://github.com/isocpp/CppCoreGuidelines) recommend against using pointers to manage dynamically allocated resources, arrays, strings and optional values, instead encouraging the use of higher-level abstractions such as [`std::unique_ptr`](http://en.cppreference.com/w/cpp/memory/unique_ptr), [`std::shared_ptr`](http://en.cppreference.com/w/cpp/memory/shared_ptr), [`std::vector`](http://en.cppreference.com/w/cpp/container/vector), [`std::array`](http://en.cppreference.com/w/cpp/container/array), [`std::array_view`](http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2014/n3851.pdf) _(not yet standardized)_, [`std::string`](http://en.cppreference.com/w/cpp/string/basic_string), [`std::string_view`](http://en.cppreference.com/w/cpp/string/basic_string_view) and [`std::optional`](http://en.cppreference.com/w/cpp/utility/optional). These types provide:

* Compile-time safety
* Run-time safety
* Documentation of intent
* Modern, easy-to-use interfaces
* Resource management

However, no such high-level types are provided by the standard to act as non-owning references to other objects; thus, pointers are still widely used in modern C++ code for this purpose, despite presenting many of the same disadvantages that led to their replacement in other use cases. This is the role played by `indirect` and `optional_indirect`:

|          | Owned                                      | Non-Owned                      |
|----------|--------------------------------------------|--------------------------------|
| Single   | `unique_ptr` `shared_ptr` `optional`       | `indirect` `optional_indirect` |
| Array    | `array` `vector` `unique_ptr` `shared_ptr` | `array_view`                   |
| String   | `string`                                   | `string_view`                  |
| Iterator | —                                          | _assorted_                     |

### Current demand

A number of existing and proposed high-level types attempt to address similar problems, but none of them adequately fit the use cases for which `indirect` and `optional_indirect` are designed. Their existence, however, shows that demand exists for solutions to the problems addressed by `indirect` and `optional_indirect`. Discussions of these other types can be found in the [design section](#design) of this proposal:

* [Why not `optional<T&>`?](#design/why-not-optional-ref)
* [Why not `reference_wrapper<T>`?](#design/why-not-reference_wrapper)
* [Why not `observer_ptr<T>`?](#design/why-not-observer_ptr)
* [Why not `not_null<T*>`?](#design/why-not-not_null)

## <a name="impact"></a>Impact on the standard

This proposal adds a single header, `<indirect>` containing both `indirect` and `optional_indirect`, which depends on the `<optional>` header for `nullopt_t`, `nullopt` and `bad_optional_access`. The `<optional>` header and all other library components will be unaffected.

The proposal could be modified to separate the definitions of `indirect` and `optional_indirect` into two headers, `<indirect>` and `<optional_indirect>` respectively, which would remove the dependency of `<indirect>` on `<optional>`. Alternatively, the `optional_indirect` class could instead be added to the `<optional>` header.

The proposed additions can be implemented using C++11 library and language features only.

## <a name="design"></a>Design decisions

When design decisions are discussed here with reference to `indirect` only, assume that the discussion applies equally to `optional_indirect` unless otherwise stated. 

### <a name="design/construction-lvalue-ref"></a>Construction from `T&`

`T&` is implicitly convertible to `indirect<T>`, so the following is valid:

```c++
int i = {};
int j = {};
indirect<int> ii = i; // `ii` references `i`
ii = j; // `ii` now references `j`
```

Given that `indirect` in many ways behaves like a pointer, and `T&` does not implicitly convert to `T*`, this behaviour may raise a few eyebrows. But what are the specific arguments against this design?

An initial objection may be that allowing conversion from `T&` and then requiring access via the indirection operators (`*` and `->`) is asymmetrical and unlike anything currently in the standard library, so we should avoid such a combination because it goes against what users have come to expect from a pointer-like type. Firstly, such asymmetrical behaviour is in fact already present in the standard library, in `optional`:

```c++
int i = {};
optional<int> oi = i;
int j = *oi;
```

If users can get used to indirection operators on a value-like type, then they can get used to construction from `T&` for a pointer-like type.

A second objection may be that "rebinding" on reassignment might be surprising behaviour; if `indirect<T>` is initiailized like `T&`, users might expect assignment from `T&` to assign to the referenced object as with assignment to `T&`. However, rebinding on assignment is already the behaviour exhibited by `reference_wrapper`:

```c++
int i = {};
int j = {};
reference_wrapper<int> ri = i; // `ri` references `i`
ri = j; // `ri` now references `j`
```

This behaviour is arguably _more_ surprising with `reference_wrapper` than with `indirect`, given the otherwise reference-like semantics of `reference_wrapper`. Indeed, logically, one would expect a pointer-like type such as `indirect` to rebind on assignment, while assignment to a reference-like type such as `reference_wrapper` could be reasonably expected to assign to the referenced object. Thus, we feel confident that users will be comfortable with the rebinding semantics of `indirect`.

But the most important question is, why enable implicit conversion from `T&` in the first place? After all, `optional` is a value-like type and `reference_wrapper` is a reference-like type; maybe construction from `T&` isn't appropriate for pointer-like types. Why not just stick with construction from `T*`? Why rock the boat? The reason is code correctness. Implicit conversion is _more convenient_ to use than explicit construction, so if a particular conversion is more "correct" than its alternatives, we _want_ to encourage its use by making it implicit.

Our logic is that because implicit conversion happens without explicit instruction from the programmer, it should be enabled _if and only if_ such a conversion can be reasonably expected to be both logically correct _and_ safe in the vast majority of cases (there are, as always, exceptions to the rule). A conversion from type `A` to type `B` is _logically correct_ if the object of type `B` represents the same kind of thing as the object of type `A`. Logical correctness is important because, even if our code is behaviourally correct, logical errors in our design can lead to bad design choices and bugs later on. Usually logical correctness is enforced by the type system, but when low-level constructs such as pointers are involved, we cannot rely on the type system to save us. A conversion is _safe_ if it never invokes undefined behaviour or throws an exception. It is obviously preferable for any operation to be safe in this way, since invoked undefined behaviour and unhandled exceptions are programming errors, but such safety is particularly important for implicit conversions because of the ease with which they can occur without the user realizing.

In our case, both `indirect<T>` and `T&` are non-owning references to objects of type `T` (the lifetime of a temporary _can_ be extended by `T const&`, but ownership cannot be transferred as it could with `T*`); they have no other meaning. Thus, implicit conversion from `T&` to `indirect<T>` is always logically correct. In addition, conversion from `T&` to `indirect<T>` is safe. Of course, `T&` _can_ theoretically reference an area of memory that doesn't contain a valid object, but by that point the user has already invoked undefined behaviour and all bets are off. Conversely, `T*` is _not_ always a non-owning reference to an object of type `T`; it could represent a dynamic resource to be managed, or an array of objects, or even an iterator. Thus, implicit conversion from `T*` to `indirect<T>` is not always logically correct. In addition, conversion from `T*` to `indirect<T>` is not safe, because `indirect<T>` has no valid state corresponding to the null state of `T*`, so constructing `indirect<T>` from a null `T*` must either invoke undefined behaviour or throw an exception. Thus, allowing implicit conversion from `T*` to `indirect<T>` would be incorrect; at most, `indirect<T>` should support _explicit_ construction from `T*` (discussed in the "[Construction from `T*`](#design/construction-ptr)" section).

If, despite the case we have just made for implicit conversion from `T&`, we decided to require explicit construction from `T&` or not enable construction from `T&` at all (assuming explicit construction from `T*` _is_ enabled), consider the calling syntax of function `void foo(indirect<int>)`:

```c++
int i = {};

foo(indirect<int>(i)); // with construction from `T&`
foo(indirect<int>(&i)); // with construction from `T*`
```

Considering that this syntactic inconvenience is likely to significantly reduce incentive to use `indirect<T>` over `T*`, the entire point of which is to encourage code correctness, and given that we consider implicit conversion from `T&` to `indirect<T>` to be logically correct, safe, and in-line with the design of existing standard library types, we see no reason why conversion from `T&` should not be enabled.

### <a name="design/conversion-lvalue-ref"></a>Conversion to `T&`

### <a name="design/construction-ptr"></a>Construction from `T*`

### <a name="design/conversion-ptr"></a>Conversion to `T*`

### <a name="design/construction-rvalue-ref"></a>Construction from `T&&`

### <a name="design/assignment"></a>Assignment operators and the `{}` idiom

### <a name="design/optional"></a>Optional API functions

### <a name="design/comparison"></a>Comparison operations

### <a name="design/move"></a>Move behaviour

### <a name="design/immutability"></a>Relationships and immutability

### <a name="design/const-propagation"></a>Const-propagation

### <a name="design/make-functions"></a>The `make` free functions

### <a name="design/cast-functions"></a>The `cast` free functions

### <a name="design/get_pointer-functions"></a>The `get_pointer` free functions

### <a name="design/propagate-const"></a>Compatibility with `propagate_const`

### <a name="design/operator-dot"></a>Use of inheritance by delegation ("`operator.` overloading")

### <a name="design/why-not-ptr"></a>Why not `T*`?

### <a name="design/why-not-ref"></a>Why not `T&`?

### <a name="design/why-not-optional-ref"></a>Why not `optional<T&>`?

### <a name="design/why-not-optional-indirect"></a>Why not `optional<indirect<T>>`?

### <a name="design/why-not-observer_ptr"></a>Why not `observer_ptr<T>`?

### <a name="design/why-not-reference_wrapper"></a>Why not `reference_wrapper<T>`?

### <a name="design/why-not-not_null"></a>Why not `not_null<T*>`?

### <a name="design/naming"></a>Naming

## <a name="technincal"></a>Technical specifications

## <a name="acknoledgements"></a>Acknowledgements

## <a name="references"></a>References
