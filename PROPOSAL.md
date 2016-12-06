# A proposal to add non-owning pointer-like class templates `indirect` and `optional_indirect`

_Joseph Thomson \<joseph.thomson@gmail.com\>_

## Introduction

`indirect<T>` and `optional_indirect<T>` are high-level pointer-like types that reference objects of type `T` without implying ownership. An `indirect<T>` is a reference to an object of type `T`, while an `optional_indirect<T>` is an _optional_ reference to an object of type `T`. Both `indirect` and `optional_indirect` have pointer-like semantics but reference-like construction and comparison syntax.

## Table of contents

* [Motivation and scope](#motivation)
  * [Why not `T*`?](#motivation/why-not-ptr)
  * [Why not `T&`?](#motivation/why-not-ref)
  * [Why not `reference_wrapper<T>`?](#motivation/why-not-reference_wrapper)
  * [Why not `optional<T&>`?](#motivation/why-not-optional-ref)
  * [Why not `observer_ptr<T>`?](#motivation/why-not-observer_ptr)
  * [Why not `not_null<T*>`?](#motivation/why-not-not_null)
* [Impact on the standard](#impact)
* [Design decisions](#design)
  * [Type conversions](#design/conversion)
    * [Conversion from `T&`](#design/conversion/from-lvalue-ref)
    * [Conversion from `T&&`](#design/conversion/from-rvalue-ref)
    * [Conversion to `T&`](#design/conversion/to-lvalue-ref)
    * [The `make` functions](#design/make)
    * [Conversion from `T*`](#design/conversion/from-ptr)
    * [Conversion to `T*`](#design/conversion/to-ptr)
    * [The `get_pointer` function](#design/get_pointer)
  * [Assignment operators and the `{}` idiom](#design/assignment)
  * [Comparison operations](#design/comparison)
  * [Move behaviour](#design/move)
  * [The `cast` functions](#design/cast)
  * [Permenant and changeable relationships](#design/relationships)
  * [Compatibility with `propagate_const`](#design/propagate-const)
  * [Use of inheritance by delegation ("`operator.` overloading")](#design/operator-dot)
  * [The case for `optional_indirect`](#design/optional)
  * [Naming](#design/naming)
* [Technical specifications](#technincal)
* [Auxiliary proposal – the `get_pointer` function](#auxiliary)
* [Acknowledgements](#acknoledgements)
* [References](#references)

## <a name="motivation"></a>Motivation and scope

Modern C++ [guidelines](https://github.com/isocpp/CppCoreGuidelines) recommend against using pointers to manage dynamically allocated resources, arrays, strings and optional values, instead encouraging the use of higher-level abstractions such as [`unique_ptr`](http://en.cppreference.com/w/cpp/memory/unique_ptr), [`shared_ptr`](http://en.cppreference.com/w/cpp/memory/shared_ptr), [`vector`](http://en.cppreference.com/w/cpp/container/vector), [`array`](http://en.cppreference.com/w/cpp/container/array), [`array_view`](http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2014/n3851.pdf) _(not yet standardized)_, [`string`](http://en.cppreference.com/w/cpp/string/basic_string), [`string_view`](http://en.cppreference.com/w/cpp/string/basic_string_view) and [`optional`](http://en.cppreference.com/w/cpp/utility/optional). These types provide:

* Compile-time safety
* Run-time safety
* Documentation of intent
* Modern, easy-to-use interfaces
* Resource management

However, no such standardized high-level types act as _non-owning_ references to other objects; thus, pointers are still widely used in modern C++ code for this purpose, despite having many of the same problems that led to their replacement in other cases. This is the role played by `indirect` and `optional_indirect`:

|          | Owned                                      | Non-Owned                      |
|----------|--------------------------------------------|--------------------------------|
| Single   | `unique_ptr` `shared_ptr` `optional`       | `indirect` `optional_indirect` |
| Array    | `array` `vector` `unique_ptr` `shared_ptr` | `array_view`                   |
| String   | `string`                                   | `string_view`                  |

A number of existing and proposed high-level types attempt to address similar problems, but we believe none of them adequately fit the use cases for which `indirect` and `optional_indirect` are designed. Their existence, however, shows that demand exists for solutions to the problems addressed by `indirect` and `optional_indirect`.

|                        | Construction | Assignment | Comparison |
|------------------------|--------------|------------|------------|
| `T`                    | value        | value      | value      |
| `optional<T>`          | value        | value      | value      |
| `T&`                   | reference    | value      | value      |
| `reference_wrapper<T>` | reference    | reference  | value      |
| `optional<T&>`         | reference    | reference  | value      |
| `T*`                   | reference    | reference  | reference  |
| `observer_ptr<T>`      | reference    | reference  | reference  |
| `not_null<T>`          | reference    | reference  | reference  |
| `indirect<T>`          | reference    | reference  | reference  |

#### <a name="motivation/why-not-ptr"></a>Why not `T*`?

Take the following data member declaration:

```c++
  widget* component;
```

We can't be sure what `component` represents, because pointers are multi-purpose. It may represent a single `widget` or an array of `widget`s; it may or may not have ownership of the `widget` it points to; and it may or may not be valid to set it to the null state. We have to look at the wider context to understand what it actually represents. However, if we use `indirect`:

```c++
  indirect<widget> component;
```

We now know _immediately_, without looking at any other code, that `component` is a _non-owning_ reference to a _single_ `widget`. In addition, we get compile-time assurances that we didn't with a pointer:

* `indirect<T>` must be initialized (it has no "null" state)
* `indirect<T>` does not define the subscript operator
* `indirect<T>` does not define arithmetic operations
* `indirect<T>` does not implicitly convert to `void*`

These assurances eliminate a variety of sources of undefined behaviour:

```c++
widget* component;     // not initialized
auto w = component[1]; // not an array
component += 1;        // not an iterator
delete component;      // not an owner
```

Some feel that these compile-time assurances aren't compelling enough to justify the use of `indirect` over `T*`, and that once all other uses of pointers have been covered by other high-level types (e.g. `unique_ptr`, `std::string`, `string_view`, `array`, `array_view`, …) the only use case that will be left for raw pointers will be as non-owning references; therefore, wherever you see a `T*` it will be safe to assume that it is "a non-owning reference to `T`". This is not true for a number of reasons:

* This is only a convention; people are not obliged to use `T*` only to mean "a non-owning reference to `T`".
* Plenty of existing code doesn't follow this convention; even if everyone followed convention from this point forward, you still cannot make the assumption that `T*` always means "a non-owning reference to `T`".
* Low-level code (new and existing) necessarily uses `T*` to mean all sorts of things; even if all new and old code followed the convention where appropriate, there would still be code where `T*` does not mean "a non-owning reference to `T`", so you _still_ cannot make that assumption.

Conversely, wherever you see `indirect<T>` in some code, you _know_ that it means "a non-owning reference to `T`". Explicitly documenting intent is generally better than letting readers of your code make assumptions about your intent.

Documentation of intent aside, we consider code correctness to be of high importance. We believe the ability to misuse pointers comes from their suboptimal design as non-owning reference types. A well-designed type is _efficient_ and has an API that is _minimal_ yet _expressive_ and _hard to use incorrectly_. Pointers as non-owning reference types may be efficient and do have a somewhat expressive API, but their API is not minimal and is very easy to use incorrectly. Case in point, this is syntactically correct but logically incorrect code:

```c++
int i = 0;
int* p = &i;
p += 1; // did I mean `*p += 1`?
*p = 42; // undefined behaviour
```

`indirect<T>` is _as_ efficient as `T*`, and has been _purpose-designed_ as a non-owning reference type to have a minimal API that is expressive and hard to use incorrectly.

#### <a name="motivation/why-not-ref"></a>Why not `T&`?

Take the following data member declaration:

```c++
  widget& component;
```

Just as with `indirect<widget>`, we know that `component` is a non-owning reference to a `widget`; there is no other meaning of `widget&`. In addition, we get all the previously listed compile-time assurances provided by `indirect<widget>`. However, `indirect<T>` offers different advantages over `T&`:

* `indirect<T>` can be "rebound" to reference different objects after construction
* `indirect<T>` can be stored in arrays and standard library containers

The inability to "rebind" `T&` also means that any class with reference data members is by default not copy assignable. This is not a problem with `indirect<T>`. The inability to store `T&` in arrays and containers stems from the fact that pointers to references are not allowed (taking the address of a reference returns a pointer to the referenced object). Since `indirect<T>` is an object type, it has no such limitation.

If we consider the design of `T&` in the same way that we considered that of `T*`, we would say that `T&` is efficient and has a minimal API that is hard to use correctly but lacks expressive power (it cannot be rebound or pointed to). `indirect<T>` is _as_ efficient as `T&`, and has been _purpose-designed_ as a non-owning reference type to have a minimal API that is expressive and hard to use incorrectly.

However, unlike `T*`, `T&` still has legitimate uses in high-level code. Indeed, the reasons to use `indirect<T>` over `T&` are the same as the reasons to use `T*` over `T&` (it's just there are further reasons to use `indirect<T>` over `T*`). This is because `indirect<T>` is semantically more similar to `T*` than to `T&`, specifically when it comes to _assignment_ and _comparison_. For example:



#### <a name="motivation/why-not-reference_wrapper"></a>Why not `reference_wrapper<T>`?



#### <a name="motivation/why-not-optional-ref"></a>Why not `optional<T&>`?

#### <a name="motivation/why-not-observer_ptr"></a>Why not `observer_ptr<T>`?

#### <a name="motivation/why-not-not_null"></a>Why not `not_null<T*>`?

## <a name="impact"></a>Impact on the standard

This proposal adds a single header, `<indirect>`, containing both `indirect` and `optional_indirect`, which depends on the `<optional>` header for `nullopt_t`, `nullopt` and `bad_optional_access`. The `<optional>` header and all other library components will be unaffected.

The proposal could be modified to separate the definitions of `indirect` and `optional_indirect` into two headers, `<indirect>` and `<optional_indirect>` respectively, which would remove the dependency of `<indirect>` on `<optional>`. Alternatively, the `optional_indirect` class could instead be added to the `<optional>` header.

The proposed additions can be implemented using C++11 library and language features only.

## <a name="design"></a>Design decisions

When design decisions are discussed here with reference to `indirect` only, assume that the discussion applies equally to `optional_indirect` unless otherwise stated. 

### <a name="design/conversion"></a>Type conversions

When deciding which type conversions to enable for `indirect`, and whether specific conversions should be _explicit_ or _implicit_, we considered both the logical correctness of a potential conversion (i.e. whether it makes sense) and its potential impact on the correctness of client code (i.e. whether it is safe). We consider a conversion to be _logically correct_ if the object before the conversion represents the _same kind of thing_ as the object after the conversion. Logical correctness should ideally be enforced by the type system, and explicit conversions often represent some violation of this enforcement; therefore, a conversion should be explicit unless it is nearly always logically correct. We consider a conversion to be _unsafe_ if it has behaviour which, if not actively accounted for, could result in programming errors in client code. Implicit conversions can easily happen without the programmer realizing, so it is imperative that they be safe.

In the case of `indirect`, we have to consider type conversions to and from the two fundamental C++ reference types, references (`T&`) and pointers (`T*`).

`indirect<T>` _always_ represents a non-owning reference to an object of type `T`, as does `T&`. Since they always represent the same kind of thing, both conversion from `T&` to `indirect<T>` and conversion from `indirect<T>` to `T&` are always logically correct. In addition, these conversions are perfectly safe, since every state of `indirect<T>` maps exactly onto every state of `T&`, and vice-versa. There are no exceptional cases for the programmer to consider. This means that implicit conversion to and from `T&` _should_ be considered.

Conversely, while `T*` _may_ represent a non-owning reference to an object of type `T`, it can also represent a number of other things:

* `T*` could represent an array
* `T*` could represent an iterator
* `T*` could represent a dynamically allocated resource to be managed

Case in point, none of the following conversions would make logical sense:

```c++
int arr[] = { 1, 2, 3 };

indirect<int> a = arr; // this wouldn't work with a `std::array`
indirect<int> b = end(arr); // doesn't point to a valid object
indirect<int> c = new int(); // resource leak
```

In addition, `T*` has an additional state that does not map onto any valid state of `indirect<T>`: the null pointer state. Therefore, when converting from a `T*` to `indirect<T>`, we can reasonably do one of two things when faced with a null pointer:

1. Throw an exception
2. Specify undefined behaviour

Both of these behaviours require the programmer to actively account for the exceptional case (by either catching the exception or by preemptively checking for a null pointer), so implicit conversion from `T*` to `indirect<T>` is out of the question. And while conversion from `indirect<T>` to `T*` _is_ safe, it is still not logically correct, so it shouldn't be implicit either; the same logic applies to conversion from `T*` to `optional_indirect<T>` which while safe (the null pointer state can map onto the empty state), is still not logically correct.

In summary, we consider enabling a particular conversion (explicit _or_ implicit) if it is likely to be logically correct _some_ of the time. We consider making a conversion implicit if it is likely to be logically correct in the _vast majority_ of cases _and_ is reasonably safe. This means that implicit conversion to and from `T&` should be considered, while conversion to and from `T*` should only be explicit. Of course, there are other things to take into account when deciding whether or not to actually enable a conversion. We discuss the specifics of particular cases in the following sections.

#### <a name="design/conversion/from-lvalue-ref"></a>Conversion from `T&`

`T&` is implicitly convertible to `indirect<T>`, so the following is valid:

```c++
int i = {};
int j = {};
indirect<int> ii = i; // `ii` references `i`
ii = j; // `ii` now references `j`
```

As detailed earlier, conversion from `T&` to `indirect<T>` is both logically correct and safe. However, given that `indirect<T>` in many ways behaves like `T*`, and `T&` does not implicitly convert to `T*`, this behaviour may raise a few eyebrows.

One concern is that allowing conversion from `T&` and then providing access via the indirection operators (`*` and `->`) is asymmetrical and unlike anything currently in the standard library, and that we should avoid this combination because it goes against what users have come to expect from a pointer-like type. However, such behaviour is already present in the standard library type `optional`:

```c++
int i = {};
optional<int> oi = i;
int j = *oi;
```

If users can get used to using "pointer-like" indirection operators in a value-like type, then we propose that they can get used to "reference-like" construction from `T&` in a pointer-like type.

Another concern is that "rebinding" on reassignment is surprising behaviour; if `indirect<T>` is initiailized like a reference, users might expect assignment from `T&` to assign to the underlying object like a reference. However, rebinding on assignment is already the behaviour exhibited by `reference_wrapper`:

```c++
int i = {};
int j = {};
reference_wrapper<int> ri = i; // `ri` references `i`
ri = j; // `ri` now references `j`
```

This behaviour is arguably _more_ surprising with `reference_wrapper` given its otherwise reference-like semantics. It could be argued that a pointer-like type such as `indirect` would be expected to rebind on assignment, while a reference-like type such as `reference_wrapper` could reasonably be expected to assign to the underlying object. Thus, we feel confident that users will be comfortable with the rebinding semantics of `indirect`.

#### <a name="design/conversion/from-rvalue-ref"></a>Conversion from `T&&`

While conversion from `T&` to `indirect<T>` is always logically correct, it is not necessarily semantically correct in the wider context. Consider this:

```c++
indirect<int const> a = 42; // equivalent to `indirect<int const> a = int{42}`
```

When the constructor of `a` returns, the temporary `int{42}` is destroyed and `a` is left _dangling_, pointing to a non-existent object. This is not a problem with `T const&` because of a C++ rule which means that the lifetime of a temporary object may be extended by binding to a const lvalue reference:

```c++
int const& a = 42; // temporary `int{42}` destroyed when `a` goes out of scope
```

Obviously, dangling references are undesirable, as they can lead to undefined behaviour. One option would be to explicitly disable construction from `T&&`; however, this would preclude passing temporary objects to functions taking `indirect<T const>` parameters:

```c++
auto f = [](indirect<foo const> x) { … };
f(make_foo()); // error: constructor `indirect<foo const>(foo&&)` is deleted 
```

This might not seem a big deal, since you could just use `T const&` instead and use `indirect<T const>` internal to your function if need be, but its impact on the usability of `optional_indirect<T const>` is far greater:

```c++
auto f = [](optional_indirect<foo const> x) { … };
foo tmp = make_foo();
f(optional_indirect<foo const>(tmp));
```

Not only must a temporary object be explicitly created, the `optional_indirect` must also be explicitly constructed, since `T*` is [not implicitly convertible](#design/conversion/from-ptr) to `optional_indirect<T>`. Compare this to the syntax required if `f` instead takes a pointer:

```c++
auto f = [](foo const* x) { … };
foo tmp = make_foo();
f(&tmp);
```

Then compare this again to the syntax required if `optional_indirect<T>` did not disable construction from `T&&`:

```c++
auto f = [](optional_indirect<foo const> x) { … };
f(make_foo());
```

If we disable construction from `T&&`, then `T*` becomes easier to use than `optional_indirect<T>`. If we do not disable construction from `T&&`, then `optional_indirect<T>` becomes easier to use than `T*`. Since use of `optional_indirect<T>` encourages more logically correct code than does use of `T*`, we want to _encourage_ instead of discourage use of `indirect`. Therefore, in balance, we have decided against disabling construction from `T&&`. Users will just have to be aware that lifetime extension of temporary objects does not occur with `indirect<T const>` as it does with `T const&`. Indeed, even disabling construction from `T&&` does not prevent the user from accidentially creating dangling references; it is just one extra (fairly obvious) case to consider on top of:

```c++
vector<int> vec = { 1, 2, 3 };
indirect<int> ii = vec[1];
vec.push_back(4); // `ii` may now be dangling
```

We also note that the designers of `basic_string_view` have not disabled construction from rvalue references; though we cannot be sure of their reasoning, it is likely to have been along similar lines:

```c++
auto get_string = []() -> string { … };
string_view v = get_string(); // `v` will be left dangling
```

#### <a name="design/conversion/from-ptr"></a>Conversion from `T*`

`indirect<T>` explicitly constructs from `T*`, throwing an `invalid_argument` exception if passed a null pointer:

```c++
int* i = get_some_pointer();

try {
    auto ii = indirect<int>(i);
} catch (invalid_argument& ex) {
    …
}
```

Since the conversion from `T*` to `indirect<T>` is not always logically correct and cannot be implemented safely, we decided that the conversion must be explicitly specified.

When considering how to handle the null pointer case, we had two choices: throw an exception or specify undefined behaviour. Throwing an exception is preferable from a safety perspective because even uncaught exceptions can be handled by a `terminate_handler`, allowing the program to shut down or recover from the error in a controlled manner; with undefined behaviour, however, the most you can do is cross your fingers and hope for the best. The only time undefined behaviour is preferable to an exception is when it results in performance gains; this is especially important in low-level library components that make up the foundations of your program (and in the case of the C++ standard library, everyone else's programs as well). In this case, if performance is a concern, and you are _sure_ that the pointer will not be null, the user can dereference the pointer instead of explicitly constructing the `indirect`:

```c++
int* i = get_some_pointer();
indirect<int> ii = *i; // you'd better be sure `i` is not null!
```

As a side note, if the source of the "not null" pointer is some legacy part of your interface that cannot be or hasn't yet been rewritten to use `indirect<T>` instead of `T*`, then you might benefit from using something like the [Guideline Support Library](https://github.com/Microsoft/GSL)'s `not_null` adapter, which provides additional run-time and compile-time assurances that a pointer is not null, while maintaining a more traditional pointer-like interface than `indirect`:

```c++
not_null<int*> i = get_some_pointer();
indirect<int> ii = *i; // `i` shouldn't be null
```

Another alternative to throwing an exception is to not support construction from `T*` at all; this way, the user is responsible for checking whether a pointer is null, as they would be when converting `T*` to `T&`. We decided against this for a number of reasons:

* No one is being forced to use the throwing constructor; the "default" operation (dereferencing `T*` then conversion from `T&` to `indirect<T>`) still has zero run-time overhead.
* Undefined behaviour is bad; if providing an alternative to pointer dereferencing which throws instead of invoking undefined behaviour results in safer client code, then this is a net win.
* Conversion of `T*` to `optional_indirect<T>` would be a huge hassle if an explicit constructor weren't provided (`auto oi = i ? *i : optional_indirect<T>()`); this would be especially annoying given that the conversion is perfectly safe; if we provide conversion from `T*` for `optional_indirect<T>` then we feels as through it should also be provided for `indirect<T>`.

#### <a name="design/make"></a>The `make` functions

The `make_indirect` free function is a factory function which takes a single `T&` and returns an `indirect<T>`. Its main purpose is to automatically deduce the template parameter of the constructed `indirect`:

```c++
int i = {};
auto ii = make_indirect(i); // `decltype(ii)` is `indirect<int>`
```

Conceptually, `make_indirect` is similar to an implicit conversion, which is why there is no overload of `make_indirect` that takes a `T*`. If we were to provide such an overload, it the logical correctness and safety of `make_indirect` would be lost:

```c++
auto ix = make_indirect(x); // this may or may not throw, depending on `decltype(ix)`
```

We _could_ provide a function to allow automatic template parameter deduction from `T*`, but it would have to be named something that conveyed its unsafe nature (something like `cast_to_indirect`). Such a function is not included in this proposal.

Even though `indirect<T>` implicitly converts to `optional_indirect<T>`, the corresponding `make_optional_indirect` function is provided for convenience.

Incidentally, the `observer_ptr` proposal specifies that the factory function `make_observer` take a `T*`. We believe this to be a mistake; `make_observer` should instead take a `T&` (though we don't recommend adding implicit conversion from `T&` given its "smart pointer"-esque design). After all, `make_unique` and `make_shared` do not take a `T*`.

#### <a name="design/conversion/to-lvalue-ref"></a>Conversion to `T&`

`indirect<T>` is neither implicitly or explicitly convertible to `T&`. Despite conversion from `indirect<T>` to `T&` being both logically correct and safe, implicit conversion to `T&` has some side effects which make it undesirable. For example, enabling implicit conversion to `T&` also enables implicit conversion to `T`, as well as other operations defined for `T`, `T&` or `T const&`:

```c++
int i = {};
indirect<int> ii = i;
ii += 1; // increments `i` 
int j = ii; // makes a copy of `i`
```

This behaviour would be expected of a reference-like type such as `reference_wrapper`, but is highly unusual for a pointer-like type such as `indirect`. Enabling arithmetic operations directly on `indirect<T>` like in this example has the potential to be particularly confusing because `indirect<T>` is in many ways like `T*`, so operations like `ii + 1` may be interpreted as a form of pointer arithmetic.

The fact that enabling implicit conversion from `indirect<T>` to `T&` causes `indirect<T>` to act like `T` could reflect that fact that implicit conversion seems to imply functional equivalence, and should be enabled with extreme care. While `indirect<T>` represents the same kind of thing as `T&`, it should not generally behave in the same way; thus, we have decided against enabling implicit conversion to `T&`.  

We also decided against enabling explicit conversion from `indirect<T>` to `T&`, as `operator*` is already performs this function:

```c++
int i = {};
indirect<int> ii = i;
int& r = *ii; // `r` references `i`
```

In addition, `optional_indirect` defines the member function `value` (borrowed from `optional`), which throws a `bad_optional_access` if the `optional_indirect` is empty. If explicit conversion to `T&` were enabled, it isn't clear whether conversion from `optional_indirect<T>` to `T&` would be `noexcept` like conversion from `indirect<T>` to `T&`, or whether it would throw like `value` and explicit construction of `indirect<T>` from `T*`. Omitting conversion to `T&` altogether means we don't have to make that somewhat arbitrary choice.

#### <a name="design/conversion/to-ptr"></a>Conversion to `T*`

`indirect<T>` explicitly converts to `T*`:

```c++
int i = {};
indirect<int> ii = i;
int* p = static_cast<int*>(ii); // `p` points to `i`
```

While conversion to `T*` is safe for both `indirect<T>` and `optional_indirect<T>`, it is not logically correct, so we decided not to make the conversion implicit. In addition, enabling implicit conversion to `T*` would enable a number of pointer operations such as `operator[]`:

```c++
int i = {};
indirect<int> ii = i;
ii[0] = 42; // `i` now equals `42`
```

This is reflective of the fact that pointers are multi-purpose types that can represent more than just non-owning references to objects.

Of course, as with [conversion _from_ `T*`](#design/conversion/from-ptr), it is always an option to not support conversion to `T*` at all. Enabling explicit conversion to `T*` is desirable mainly because it is awkward to obtain a `T*` from an `indirect<T>` and _especially_ from an `optional_indirect<T>`:

```c++
int i = {};
indirect<int> ii = i;
optional_indirect<int> oi = i;
int* p = &*ii;
int* q = oi ? &*oi : nullptr;
```

### <a name="design/get_pointer"></a>The `get_pointer` function

The `get_pointer` free function is provided as a convenient alternative to explicit conversion from `indirect<T>` to `T*`; the advantage of `get_pointer` over `static_cast<T*>` is that the pointer type is deduced automatically:

```c++
int i = {};
indirect<int> ii = i;
auto p = get_pointer(ii); // `decltype(p)` is `int*`
```

Current standard library smart pointer types `unique_ptr` and `shared_ptr` both provide a `get` member function to serve this purpose. We were concerned that the name `get` for a function returning a raw pointer would be both insufficiently descriptive for a type whose name does not contain the suffix `_ptr` (see the ["Naming" section](#design/naming)), and have unexpected behaviour for a type which is likely to be initialized from `T&` rather than `T*`, especially for users who are familiar with `reference_wrapper`, whose `get` function returns `T&`:

```c++
int i = {};

reference_wrapper<int> ri = i;
auto x = ri.get(); // `decltype(x)` is `int`

indirect<int> ii = i;
auto y = ii.get(); // `decltype(y)` is `int*`
```

Given that `indirect` already enables [explicit conversion to `T*`](#design/conversion/to-ptr), we felt that providing a free function was most appropriate, and the name `get_pointer` seemed descriptive and similar enough to the name `get` to imply equivalent functionality.

Regardless of whether this proposal gains traction, we believe that adding `get_pointer` overloads for all pointer-like standard library types would be useful for generic programming in general, although [compatibility with `propagate_const`](#design/propagate_const) was the specific use case that lead to its conception. We include an [auxiliary proposal](#auxiliary) for its inclusion which can be considered independently of the proposal for `indirect`. 

### <a name="design/assignment"></a>Assignment operators and the `{}` idiom

`indirect` and `optional_indirect` do not explicitly define any assignment operators. This enables automatic support of the `{}` idiom for resetting an object to its default state:

```c++
int i = {};

optional_indirect<int> oi = i;
oi = {}; // `oi` is now empty

indirect<int const> ii = i;
ii = {}; // error: cannot default construct `indirect`
```

If assignment operators were explicitly implemented, extra (non-trivial) measures would need to be taken to ensure that this idiom worked correctly. Since `indirect` and `optional_indirect` are likely to be implemented as a simple pointer, and default copy and move assignment operators are generated, the compiler should be able to easily avoid the additional copy using the "as if" rule, so there should be no performance penalty for this design choice.

### <a name="design/comparison"></a>Comparison operations

Naturally, the full range of comparison operators are defined for `indirect` (as well as a `hash` implementation). Given that `T&` implicitly converts to `indirect<T>`, we also provide equality and inequality comparison operators for comparison of `indirect<T>` with `T&`:

```c++
int i = {};
int j = {};
indirect<int> ii = i;
assert(ii == i);
assert(ii != j);
```

The signatures of these equality comparison functions look something like this:

```c++
template <typename T1, typename T2>
bool operator==(indirect<T1> const& lhs, T2& rhs);
```

They only partake in overload resolution if `T1*` is convertible to `T2*` or vice-verca.

There is likely to be some concern that making an equality comparison of `T&` compare the address of the reference object is surprising behaviour. Indeed, there is no existing instance of this kind of behaviour in the standard. However, we believe that this behaviour poses no more risk than allowing [assignment from `T&`](#design/conversion/from-lvalue-ref) to assign the address of the referenced object (which is already the behaviour of `reference_wrapper`). Once users have familiarized themselves with `indirect`, we believe that this behaviour will seem quite natural.

### <a name="design/move"></a>Move behaviour

Neither `indirect` not `optional_indirect` not define distinct move behaviour (moving is equivalent to copying); this behaviour is shared by `optional`. It could be argued that a moved-from `optional_indirect` should become disengaged, but in reality we have no way of knowing how client code wants a moved-from `optional_indirect` to behave, and such behaviour would incur a potentially unnecessary run-time cost. In addition, a moved-from `indirect` cannot be disengaged, so having different behaviour for `optional_indirect` would be potentially surprising.

An additional advantage of keeping the compiler-generated move operations is that `indirect` and `optional_indirect` can be [trivially copyable](http://en.cppreference.com/w/cpp/concept/TriviallyCopyable) (they can be copied using [`std::memcpy`](http://en.cppreference.com/w/cpp/string/byte/memcpy), a performance optimization that some library components employ).

### <a name="design/cast"></a>The `cast` functions

Given that `indirect` is intended to replace the use of pointers in many places, we felt that the ability to cast would be missed. Thus, we provide three sets of `cast` functions:

* `static_indirect_cast`
* `dynamic_indirect_cast`
* `const_indirect_cast`

The naming convention follows that established by the `shared_ptr` case functions (e.g. `static_pointer_cast`). Overloads are provided for both `indirect` and `optional_indirect`. We chose not to provide a `reinterpret_indirect_cast` as `indirect` is a high-level type and `reinterpret_cast` really only makes sense when manipulating the low-level representation of objects in memory.

The cast operations are implemented in terms of the corresponding `static_cast`, `dynamic_cast` and `const_cast` operations for `T&` and `T*` for `indirect` and `optional_indirect` respectively. In particular, this means that `dynamic_indirect_cast` will throw an exception in the cast of an invalid cast of an `indirect` and will return an empty `optional_indirect` in the case of an invalid cast of an `optional_indirect`.

### <a name="design/relationships"></a>Permenant and changeable relationships

We have seen some people state that they use `T&` to model permenant relationships and `T*` to model relationships that can change. This descriptive ability is not lost with `indirect`; in fact, it is enhanced. The `const` mechanism provides a natural way to model permenancy in C++. `indirect` and `optional_indirect` provide a natural way to model required and optional relationships. Combining these gives greater flexibility than `T&` or `T*` can alone:

* `indirect<T>` – a required, changeable relationship
* `indirect<T> const` – a required, permanent relationship (equivalent to `T&`)
* `optional_indirect<T>` – an optional, changeable relationship (equivalent to `T*`)
* `optional_indirect<T> const` – an optional, permanent relationship

### <a name="design/propagate-const"></a>Compatibility with `propagate_const`

The term _const-propagation_ refers to the behaviour whereby the read-only status of an object (as denoted by the `const` keyword) is forwarded it its constituent components. Const-propagation occurs by default from an object to its members:

```c++
struct foo {
    int bar = {};
};

foo const f;
f.bar = 42; // error: `foo::bar` is read-only
```

This const-propagation can be disabled using the `mutable` keyword:

```c++
struct foo {
    mutable int bar = {};
};

foo const f;
f.bar = 42; // a-okay
```

However, const-propagation does not occur in the case of reference data members:

```c++
struct foo {
    int& bar;
};

int i = {};
foo const f = { i };
f.bar = 42; // a-okay
```

Nor does it occur from pointer to pointee:

```c++
struct foo {
    int* bar;
};

int i = {};
foo const f = { &i };
*f.bar = 42; // a-okay
```

We have decided that `indirect` should not exhibit const-propagation behaviour for a number of reasons:

* _Consistency_: existing pointer-like types `unique_ptr` and `shared_ptr` do not exhibit const-propagation.
* _Flexibility_: there is no obvious way to opt-out of const-propagation if it is built into a type by default.
* _Convenience_: correct const-propagation behaviour necessarily requires copy operations to be disabled.
* _Correctness_: we view const-propagation as a _value-like_ behaviour while indirect is a _pointer-like_ type.

Instead we are opting to instead make `indirect` compatible with the proposed [`propagate_const`](http://en.cppreference.com/w/cpp/experimental/propagate_const) interface adapter. Unfortunately, `propagate_const` as it stands is not compatible with `indirect`. Thus, there are two options: either change the design of `indirect` to work with `propagate_const`, or change the design of `propagate_const` to work with `indirect`.

For `indirect` to work with `propagate_const` as it stands, we would need to make two changes:

* Add a member function `get` which returns a pointer to the referenced object.
* Enable explicit conversion from `indirect` to `bool`.

Our objections to a `get` member function are detailed in our [discussion of the `get_pointer` function](#design/get_pointer), and while conversion to `bool` makes sense for `optional_indirect`, it makes little sense for `indirect`, and we feel its inclusion would be misleading for users who might understandably think that its presence indicates that `indirect` posesses some kind of "empty" state. Thus, we would rather not make these changes just to accommodate `propagate_const`. We instead propose a couple of changes to the design of `propagate_const`:

* Depend on the `get_pointer` free function, not a `get` member function, for provision of a pointer to the underlying object; `propagate_const<T>` should provide a const-propagating `get` member function only if `T` has a function called `get` with the appropriate signature.
* Remove the requirement that `T` convert to `bool`; conversion from `propagate_const<T>` to `bool` should be enabled only if `T` converts to `bool`.

It may seem presumptuous of us to suggest that another proposal be adjusted to accommodate our needs, but we believe the current incompatibilities are indicative of overly restrictive requirements on the type `T` on the part of `propagate_const`. The job of `propagate_const` is to adapt the interface of an existing pointer-like type `T` to have const-propagation semantics. Conversion to `bool` is an extraneous requirement for const-propagation. Imposing a required interface beyond the existence of `operator*` and `operator->` is also unnecessary; a non-member function such as `get_pointer` is a far less invasive mechanism that follows the philosophy of other free functions such as `begin` and `end`, which are intended to allow the non-invasive adaptation of non-standard types to work with standard generic algorithms.

We believe that our suggested changes should benefit the users of `propagate_const` regardless of whether this proposal is accepted. We also propose some other modifications which, while inessential, should further facilitate operation with `indirect` and improve the usability of `propagate_const` in general:

* Conversion to pointer to `element_type*` should be explicit if conversion from `T` to `element_type*`; currently, only implicit conversion is supported.
* Implicit conversion to `T` and the const version of `T` should be supported, as this kind of conversion is only currently supported for the `propagate_const<T*>` specialization (via conversion to `element_type*`). This would require a mechanism by which `propagate_const` can determine the const version of an arbitrary type `T`. We suggest that an additional requirement be made of `T`: the existence of a member type `T::const_type` (for example, `unique_ptr<T>::const_type` would be `unique_ptr<T const>`); we have provided `const_type` for indirect in this proposal.

A sample implementation of a version of `propagate_const` with these changes can be found [here](https://github.com/hpesoj/cpp-indirects/blob/master/tests/propagate_const.hpp).

### <a name="design/operator-dot"></a>Use of inheritance by delegation ("`operator.` overloading")

### <a name="design/optional"></a>The case for `optional_indirect`

### <a name="design/naming"></a>Naming

## <a name="technincal"></a>Technical specifications

## <a name="auxiliary"></a>Auxiliary proposal – the `get_pointer` function

## <a name="acknoledgements"></a>Acknowledgements

## <a name="references"></a>References
