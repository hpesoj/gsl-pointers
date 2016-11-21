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

Modern C++ guidelines recommend using high-level abstractions such as [`std::vector`](http://en.cppreference.com/w/cpp/container/vector), [`std::array`](http://en.cppreference.com/w/cpp/container/array), [`std::array_view`](http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2014/n3851.pdf) _(not yet standardized)_, [`std::string`](http://en.cppreference.com/w/cpp/string/basic_string), [`std::string_view`](http://en.cppreference.com/w/cpp/string/basic_string_view), [`std::unique_ptr`](http://en.cppreference.com/w/cpp/memory/unique_ptr), [`std::shared_ptr`](http://en.cppreference.com/w/cpp/memory/shared_ptr) and [`std::optional`](http://en.cppreference.com/w/cpp/utility/optional) instead of raw pointers wherever possible. However, there is one major use of raw pointers that currently lacks a corresponding standardized high-level type: non-owning references to single objects. This is the gap filled by `indirect` and `optional_indirect`:

|          | Owned                                      | Non-Owned                              |
|----------|--------------------------------------------|----------------------------------------|
| Single   | `unique_ptr` `shared_ptr` `optional`       | `indirect` `optional_indirect`         |
| Array    | `array` `vector` `unique_ptr` `shared_ptr` | `array_view`                           |
| String   | `string`                                   | `string_view`                          |
| Iterator | —                                          | _assorted_                             |

An [existing proposal](http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2014/n4282.pdf) for `unique_ptr`-esque "dumb" pointer type [`observer_ptr`](http://en.cppreference.com/w/cpp/experimental/observer_ptr) aims to address the "non-owned single" use case, but `indirect` and `optional_indirect`, rather than being based on the owning smart pointer types that were designed to fill the "owned single" gap, are designed specifically for their intended purpose. The result is an API that:

* Improves code correctness
* More clearly documents intent
* Has more natural usage syntax

## <a name="impact"></a>Impact on the standard

## <a name="design"></a>Design decisions

### <a name="design/construction-lvalue-ref"></a>Construction from `T&`

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
