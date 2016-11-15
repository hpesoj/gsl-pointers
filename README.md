# `view` and `optional_view`: single object views for C++

`view` and `optional_view` are types used to indirectly reference another object without implying ownership. `view<T>` is a mandatory view of an object of arbitrary type `T`, while `optional_view<T>` is an optional view of an object of arbitrary type `T`. Both `view` and `optional_view` have reference-like initialization semantics and pointer-like indirection semantics.

## Contents

* [Motivation](#motivation)
* [Quick Example](#example)
* [Tutorial](#tutorial)
* [Design Rationale](#rationale)
* [FAQ](#faq)

## Motivation

Modern C++ guidelines recommend using high-level abstractions such as [`std::vector`](http://en.cppreference.com/w/cpp/container/vector), [`std::array`](http://en.cppreference.com/w/cpp/container/array), `std::array_view` _(not yet standardized)_, [`std::string`](http://en.cppreference.com/w/cpp/string/basic_string), [`std::string_view`](http://en.cppreference.com/w/cpp/string/basic_string_view), [`std::unique_ptr`](http://en.cppreference.com/w/cpp/memory/unique_ptr), [`std::shared_ptr`](http://en.cppreference.com/w/cpp/memory/shared_ptr) and [`std::optional`](http://en.cppreference.com/w/cpp/utility/optional) instead of raw pointers wherever possible. However, there is one major use of raw pointers that currently lacks a corresponding standardized high-level type: non-owning references to single objects. This is the gap filled by `view` and `optional_view`:

|          | Owned                                      | Non-Owned                      |
|----------|--------------------------------------------|--------------------------------|
| Single   | `unique_ptr` `shared_ptr` `optional`       | `view` `optional_view`         |
| Array    | `array` `vector` `unique_ptr` `shared_ptr` | `array_view`                   |
| String   | `string`                                   | `string_view`                  |
| Iterator | —                                          | _assorted_                     |

An [existing proposal](http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2014/n4282.pdf) for `unique_ptr`-esque "dumb" pointer type [`observer_ptr`](http://en.cppreference.com/w/cpp/experimental/observer_ptr) aims to address the "non-owned single" use case, but `view` and `optional_view`, rather than being based on the owning smart pointer types that were designed to fill the "owned single" gap, are designed specifically for their intended purpose. The result is an API that:

* Improves code correctness
* More clearly documents intent
* Has more natural usage syntax

## <a name="example"></a>Quick Example

Take an example where a pointer might be used to represent an optional, non-owning relationship between a person and their animal companion (you can't, like, _own_ an animal, man):

```c++
struct person {
    animal* pet = {};
};

animal fluffy;
person bob;
bob.pet = &fluffy;
```

An `optional_view` can be used in place of the pointer:

```c++
struct person {
    optional_view<animal> pet;
};

animal fluffy;
person bob;
bob.pet = fluffy; // note: initialized from `animal&` instead of `animal*`
```

The `optional_view` can be tested for "empty" status as if it were a `bool`, and set to "empty" by assigning `{}` just like a pointer or [`std::optional`](http://en.cppreference.com/w/cpp/utility/optional):

```c++
assert(bob.pet);
bob.pet = {};
assert(!bob.pet);
```

A mandatory relationship can be expressed by using `view` instead of `optional_view`:

```c++
struct person {
    view<animal> pet;
};

animal fluffy;
person bob = { fluffy }; // note: initialization required on construction
```

Like pointers, both `view` and `optional_view` use the indirection operators (`*` and `->`) to access the referenced object:

```c++
bob.pet->sleep();
```

And `view` also implicitly converts to `T&`:

```c++
animal dolly = bob.pet; // note: no need to "dereference"
```

Both `view` and `optional_view` can be copied freely, and `view` implicitly converts to `optional_view`:

```c++
view<animal> pet = bob.pet;
optional_view<animal> possible_pet = pet;
```

## <a name="tutorial"></a>Tutorial

In this demonstration, we will create a type that can be used to create a tree of connected nodes. Nodes will only keep non-owning references to other nodes. The client is responsible for the lifetime of each node. Each node will have an optional parent and zero or more children. For simplicity, we will not attempt to prevent cycles from being formed.

With only pointers and references in our arsenal, we might start with something like this:

```c++
class node {
private:
    node* parent;

public:
    void set_parent(node* new_parent) {
        parent = new_parent;
    }

    node* get_parent() const {
        return parent;
    }
};
```

We make `parent` a pointer because it is natural to use the null pointer state to represent the lack of a parent. However, we already have a bug in our code: we forgot to initialize `parent`! In addition, the meaning of `node*` is not 100% clear. We want it to mean "_non-owning_, _optional_ reference to a _single_ `node`". However, pointers have other potential uses:

* Pointers can be used in place of references, where they are assumed to _not_ be null; a violation of this condition is a programming error and may result in undefined behaviour.
* Pointers can be used to represent arrays of objects; indeed, pointers define the subscript operator (e.g. 'p[i]') for indexing into arrays; however, using an index greater than zero with a pointer that references a single object is undefined behaviour.
* Pointers can be used to represent [random-access iterators](http://en.cppreference.com/w/cpp/concept/RandomAccessIterator); indeed, pointers define the arithmetic operators (e.g. 'p++') used to move random-access iterators; however, moving and then dereferencing a pointer that references a single object is undefined behaviour.
* A dynamically created/allocated object, array or system resource referenced by a pointer may be implicitly owned by one or more holders of a copy of the pointer; indeed, pointers implicitly convert to `void*` so they can be used in expressions such as `delete p` or `free(p)`; failure to correctly destroy/deallocate such an object, array or resource may result in either a resource leak or undefined behaviour.

With all this potential undefined behaviour, it is important that it is obvious to the reader what `node*` _represents_. We _could_ provide supplementary documentation, but if we replace `node*` with `optional_view<node>`, we can convey our intentions automatically using the type system. An `optional_view<T>` _is_ a _non-owning_, _optional_ reference to a _single_ object of type `T`: an optional view of `T`. With `optional_view<T>`, we also get compile-time assurances that we didn't with `T*`:

* `optional_view<T>` is always default initialized (to its empty state)
* `optional_view<T>` does not define the subscript operator (it isn't an array)
* `optional_view<T>` does not define arithmetic operations (it isn't an iterator)
* `optional_view<T>` does not implicitly convert to `void*` (it doesn't "own" anything)

So let's replace `node*` with `optional_view<node>`:

```c++
class node {
private:
    optional_view<node> parent;

public:
    void set_parent(optional_view<node> new_parent) {
        parent = new_parent;
    }

    optional_view<node> get_parent() const {
        return parent;
    }
};
```

Note that with `optional_view`, the calling syntax changes slightly. `optional_view<T>` is no longer implicitly constructible from `T*`, since this conversion may not always be conceptually or even semantically correct since pointers can represent many things. Instead, `optional_view<T>` is implicitly constructible from `T&`; this means that we `set_parent` and test the result of a call to `get_parent` _without_ taking the address of the `node`s:

```c++
node a, b;
b.set_parent(a)
assert(b.get_parent() == a);
```

This is arguably a more natural syntax; indeed, references, not pointers, are the most natural reference types in C++.

We use `{}` to specify the empty state and conversion to `bool` to test for the empty state, just as with a pointer or [`std::optional`](http://en.cppreference.com/w/cpp/utility/optional):

```c++
b.set_parent({})
assert(!b.get_parent());
```

Now, it would be nice if `node` also kept track of its children so that we can navigate both up _and_ down the tree. Using only references and pointers, we might initially consider storing a `std::vector` of references:

```c++
    std::vector<node&> children;
```

Alas, this will not compile. References have irregular copy behaviour: unlike with pointers, copy assignment applies directly to the referenced object, not to the reference itself. This behaviour precludes storage of references in STL containers like `std::vector`; instead, we are forced to store pointers:

```c++
private:
    …
    std::vector<node*> children;

public:
    …

    std::size_t get_child_count() const {
        return children.size();
    }

    node& get_child(std::size_t index) const {
        return *children[index];
    }

private:
    void add_child(node& child) {
        children.push_back(&child);
    }

    void remove_child(node& child) {
        children.erase(std::find(children.begin(), children.end(), &child));
    }
```

Now, we could replace `node*` with `optional_view<node>`, but this is misleading since children are _not_ optional: `node*` should never be null and `optional_view<node>` should never be empty. So instead of `node&` or `node*`, we can use `view<node>`, the mandatory counterpart to `optional_view<node>`. In addition to all the benefits that `optional_view` provides, `view`  _must_ always reference an object: it has no empty state. A `view<T>` _is_ a _non-owning_, _mandatory_ reference to a _single_ object of type `T`: a view of `T`. And it can be stored in STL containers, just like `optional_view<T>` or `T*`.

Let's replace all instances of `node&` and `node*` with `view<node>`, and add the calls to `add_child` and `remove_child` to `set_parent`:

```c++
private:
    …
    std::vector<view<node>> children;

public:
    void set_parent(optional_view<node> new_parent) {
        if (parent) parent->remove_child(*this);
        parent = new_parent;
        if (parent) parent->add_child(*this);
    }

    …

    view<node> get_child(std::size_t index) const {
        return children[index];
    }

private:
    void add_child(view<node> child) {
        children.push_back(child);
    }

    void remove_child(view<node> child) {
        children.erase(std::find(children.begin(), children.end(), child));
    }
};
```

We now have compile-time guarantees that we didn't have previously. Note that the syntax and semantics of `view<T>` are notably different from those of `T&`. For example, when using `auto` type deduction, copying a `T&` copies the referenced object by default, while copying a `T*` copies the pointer by default:

```c++
auto aa = b.get_parent(); // `decltype(aa)` is `node*`
auto& bb = a.get_child(0); // `decltype(bb)` is `node&`
auto cc = a.get_child(0); // `decltype(cc)` is `node`
```

And `T&` and `T*` use different syntax to access the referenced object:

```c++
bb.set_parent({}); // `T&` uses `.`
aa->set_parent(&bb); // `T*` uses `->`
```

Conversely, both `view<T>` and `optional_view<T>` have pointer-like copying and indirection semantics:

```c++
auto aa = b.get_parent(); // `decltype(aa)` is `optional_view<node>`
auto bb = a.get_child(0); // `decltype(bb)` is `view<node>`

bb->set_parent({}); // `view<T>` uses `->`
aa->set_parent(bb); // `optional_view<T>` uses `->`
```

`view<node>` _can_ implicitly convert to `node&` if the type is specified, though indirection syntax must still be used to access the referenced object if conversion isn't triggered:

```c++
node& bb = a.get_child(0);
a.get_child(0)->set_parent({});
```

## <a name="rationale"></a>Design Rationale

### <a name="rationale-construction-from"></a>Construction from and conversion to `T&` and `T*`

Both `view` and `optional_view` are constructible from and convertible to `T&` and `T*`, but only construction of `view<T>` and `optional_view<T>` from `T&` and conversion from `view<T>` to `T&` are _implicit_. The idea is that conversions should be implicit if they are functionally equivalent and conversion is safe virtually all of the time, while they should be explicit if the types are not always functionally equivalent or conversion is safe only some of the time ("safe" here means not invoking undefined behaviour _and_ being `nothrow`). `T&` should always represent a valid "view" of an object in a well-formed C++ program, so implicit construction of `view<T>` and `optional_view<T>` from `T&` is correct; conversion from `optional_view<T>` to `T&` is not safe as the `optional_view<T>` may be empty, so conversion must be explicitly specified. Conversely, `T*` may or may not be a valid reference to an object; for example:

* `T*` may have ownership semantics
* `T*` may represent an array
* `T*` may be an iterator (maybe even a "past-the-end" iterator)

In none of these cases would it be correct to allow either `view<T>` or `optional_view<T>` to be implicitly constructed from  or converted to `T*`. It is up to the programmer to explicitly specify these conversions when they are sure it is correct to do so.

It's worth noting that if conversion from `optional_view<T>` to `T*` were implicit, then array-like operations such as `v[i]`, iterator-like operations such as `v++`, and ownership operations such as `delete v` would automatically be enabled. This could be considered reflective of the fact that implicit conversion implies functional equivalence.

Also note that there are no overloads of `make_view` or `make_optional_view` that take a pointer, as they are merely intended to facilitate automatic type deduction, and are conceptually similar to implicit constructors.

### <a name="rationale-construction-from-pointer-to-view"></a>Construction of `view` from `T*` or `optional_view`

Explicit construction of `view` from a pointer or an `optional_view` _is_ supported and throws a `std::invalid_argument` if the pointer is null or the `optional_view` is empty. It could be argued that such a feature is not strictly required as part of a minimal and efficient API (the user could implement equivalent behaviour just as efficiently as a non-member function) and encourages programming errors as the user may not realize that constructing a `view` from a pointer is not `nothrow`. However, the explicit nature of the conversion means that the user is unlikely to do it by accident:

```c++
    foo(view<int>(p));
```

Thus, even though such functionality isn't strictly necessary, it is a convenient way to safely (i.e. without invoking undefined behaviour) convert a pointer or `optional_view` to a `view`. It also seems appropriate to support such operations given the inclusion of the [throwing accessor function](#rationale-named-member-functions) `value` in `optional_view`, a function which is also strictly not required and is inspired by the design of `std::optional`.

### <a name="rationale-construction-from-rvalue"></a>Construction from `T&&`

Initialization of `view` and `optional_view` from rvalues is allowed. This means that it is possible to pass temporary objects to functions taking `view<T const>` or `optional_view<T const>`:

```c++
void foo(view<int const> i) { … }

int i = 42;

foo(i); // okay
foo(42); // also okay
```

However, unlike a reference-to-const, a `view<T const>` does not extend the lifetime of the temporary from which it is constructed. Thus, while this is safe:

```c++
int const& i = 42; // lifetime of temporary extended by `i`
std::cout << i; // a-okay
```

This is most definitely not safe:

```c++
view<int const> i = 42; // temporary destroyed after assignment
std::cout << *i; // undefined behaviour!
```

Some might consider this grounds to disable construction from `T&&` as in [`std::reference_wrapper`](http://en.cppreference.com/w/cpp/utility/functional/reference_wrapper). However, `std::string_view` sets a precedent by allowing such behaviour:

```c++
std::string_view v = std::string("hello, world");
std::cout < v; // undefined behaviour
```

Indeed, prohibiting passing of temporaries as "view" arguments would be extremely limiting. The user must simply be aware that lifetime extension does not occur with such types.

### <a name="rationale-assignment"></a>Assignment operators and the `v = {}` idiom

Neither `view` nor `optional_view` explicitly define any assignment operators. This enables automatic support of the `v = {}` idiom for resetting an object to its default state:

```c++
optional_view<int> v1 = i;
v1 = {}; // `v1` is now empty

view<int const> v2 = i;
v2 = {}; // compile error (`view` has no default state)
```

If assignment operators were explicitly implemented, extra measures would need to be taken to ensure that `v = {}` compiled for `optional_view` and didn't compile for `view`. Since both types are very lightweight, the compiler should be able to easily elide the additional copy, so there should be no performance penalty for this design choice.

### <a name="rationale-move"></a>Move behaviour

Neither `view` not `optional_view` define distinct move behaviour: moving is equivalent to copying. It could be argued that a moved-from `optional_view` should become empty, but in reality we have no way of knowing how client code wants a moved-from `optional_view` to behave, and such behaviour would incur a potentially unnecessary run-time cost. In addition, a moved-from `view` cannot be empty, so having different behaviour for `optional_view` would be potentially surprising. An additional advantage of keeping the compiler-generated move operations is that `view` and `optional_view` can be [trivially copyable](http://en.cppreference.com/w/cpp/concept/TriviallyCopyable) (they can be copied using [`std::memcpy`](http://en.cppreference.com/w/cpp/string/byte/memcpy), a performance optimization that some library components employ).

### <a name="rationale-get"></a>Compatibility with `std::experimental::propagate_const`

It has been decided that `view` and `optional_view` should not try to "propagate constness" to the referenced object; for example, `view<T>::operator*() const` should return `T&` not `T const&` (and there should not exist a non-const overload). Instead, `view` and `optional_view` are designed to be compatible with the proposed [`std::experimental::propagate_const`](http://en.cppreference.com/w/cpp/experimental/propagate_const) interface adapter. Unfortunately, `propagate_const` has been designed to work with "pointer-like" types, and `view` and `optional_view` are not "pointer-like" in the conventional sense.

First, `propagate_const` requires compatible types to implement a member function named `get` which returns the object's underlying raw pointer. While `get` may be a great name for such a function in a smart pointer, it is terribly undescriptive for a view type. It would be better if conversion to pointer were left to the explicit conversion operator.

Secondly, `propagate_const` requires compatible types to be convertible to `bool`. While this is no problem for `optional_view`, it really makes no sense for `view` to convert to bool. It would be better if this weren't a requirement of `propagate_const` at all.

Thirdly, `propagate_const` works well with raw pointers because it implicitly converts to `T*` and `T const*`. However, `propagate_const<T>` doesn't implicitly convert to `T` or the const version of `T` for an arbitrary `T`. This could be supported if `propagate_const` had some way of knowing what the const version of `T` was for an arbitrary `T`. 

Fortunately, `propagate_const` has not yet been standardized, so there we have a chance to fix things. In order to allow `propagate_const` to work with all types that have indirection semantics, the following changes are suggested:

* Add a `std::get_pointer` free function that obtains the underlying pointer for various standard library types. `propagate_const` should rely on this function rather than a `get` member function. Note that Boost provides a [similar function](www.boost.org/doc/libs/release/boost/get_pointer.hpp) already.
  * `get_pointer(T* p)` will return `p`
  * `get_pointer(unique_ptr<T> const& p)` and `get_pointer(shared_ptr<T> const& p)` will return `p.get()`
  * `get_pointer(view<T> const& v)` and `get_pointer(optional_view<T> const& v)` will return `static_cast<T*>(v)`
  * `get_pointer(propagate_const<T>& pc)` and `get_pointer(propagate_const<T> const& pc)` will return `get_pointer(pc.t)`, where `pc.t` is the underlying object of type `T`
* Enable `propagate_const<T>::operator bool` _only_ if `T::operator bool` exists.
* Require compatible types to provide a member type `const_type` (for example, `view<T>::const_type` would be `view<T const>`). This information can then be used to implement implicit conversion to `T` and `T::const_type`.

A sample implementation of a version of `propagate_const` with these changes can be found [here](https://github.com/hpesoj/cpp-views/blob/master/tests/propagate_const.hpp).

### <a name="rationale-optional"></a> Use of `optional_view<T>` rather than `std::optional<view<T>>`

It could be argued that the role of an optional "view" should be played by `optional<view<T>>`. After all, there is no `std::optional_string_view` to act as a higher-level version of a `char const*` which represents an optional string; in fact, it could be argued that `char const*` should never be conceptually never be null, which is why types such as `std::optional_string_view` don't exist. Surely the same applies to a `T*` that represents a reference?

In practice, it is common to use `T*` as an optional reference and `T&` as a mandatory reference, while a `char const*` that represents a string is more commonly expected to not be null. In addition, the double indirection syntax required when using `optional<view<T>>` is very clunky:

```c++
foo f;
auto v = make_optional(make_view(f));
(*v)->bar = 42;
```

Also, `optional<view<T>>` generally occupies _twice_ the memory of `optional_view<T>`, which may put people off of using such a simple type.

### <a name="rationale-nullopt"></a>Reuse of `std::nullopt` and `std::bad_optional_access`

Given that `optional_view<T>` is conceptually similar to `std::optional<view<T>>`, it makes sense to reuse features standardized with `std::optional`.

The current implementation provides its own `nullopt_t`, `nullopt` and `bad_optional_access`, since implementations of `std::optional` are not yet widely available.

### <a name="rationale-named-member-functions"></a>Supplementary accessors (`value` and `value_or`)

The optional value type [`std::optional`](http://en.cppreference.com/w/cpp/utility/optional) provides the throwing accessor function `value` and the convenience accessor function `value_or`. Neither of these functions are necessarily part of a minimal and efficient API (they could be just as efficiently implemented as non-member functions), but presumably they were felt to be useful enough to include anyway. `optional_view` implements corresponding functions which work in roughly the same way. The only difference is that `optional` has rvalue overloads that move the contained value out of the `optional` (e.g. `t = std::move(op).value()` will move the value out of `op` and into `t`). `optional_view` does not own the value it references, so it does not assist in such operations. Thus, `value` always returns an lvalue reference and `value_or` always performs a copy.

### <a name="rationale-naming"></a>Naming

The names `view` and `optional_view` are based on conventions already set by the C++ standard library, namely:

* `std::string_view` – a non-owning "view" of a string
* `std::optional` – an optional value type

The term "view" seems to be used to refer to an object that allows the whole or part of another object or data structure to be examined without controlling the lifetime of said object or data structure (much like a reference). This seems like an accurate description `view` and `optional_view`.

The terms "reference" and "pointer" are inappropriate since they are overly general and already have established meaning in the C++ lexicon. The term "view" seems more appropriate as it describes the function of the feature rather than how it is likely to be implemented.

The term "observer" could potentially be used (as in `std::experimental::observer_ptr`); there is no arguing that it is descriptive of what the classes do. However, the term "view" has already been enshrined in the C++ standard, and there is also the concern that there will be confusion with the unrelated ["observer pattern"](https://en.wikipedia.org/wiki/Observer_pattern), a software design pattern that is widely used in C++ codebases. There is also the minor benefit that "view" is four characters less to type than "observer".

The term "optional" has an obvious meaning. Indeed, an `optional_view<T>` is pretty much functionally equivalent to an `optional<view<T>>`; the former is simply a bit nicer to use.

One possible question is, should the "optional" type be the default? In other words, should `optional_view` be named `view` and `view` be named something like `mandatory_view` or `compulsory_view`? Aside from the precident set by `std::optional` (which I believe is pretty strong by itself), I think the answer is "no": `view` is inherently a simpler type; there is no need to account for the additional "empty" state which, like null pointers, is a major source of potential programming errors. The mandatory type should be the default choice, and the optional type should be opted-into. The naming of the types should reflect this.

## <a name="faq"></a>FAQ

### <a name="faq-good-enough"></a>Aren't `T*` and `T&` good enough?

The suggestion has been made that raw pointers work just fine as optional view types, and there is no need to introduce new types when we already have long-established types that are well-suited to this role.

Some argue that once all other uses of pointers have been covered by other high-level types (e.g. `std::unique_ptr`, `std::string`, `std::string_view`, `std::array`, `std::array_view`, …) the only use case that will be left for raw pointers will be as optional view types, so it will be safe to assume that wherever you see `T*` it means "an optional view of `T`". This is not true for a number of reasons:

* This is only a convention; people are not obliged to use `T*` only to mean "an optional view of `T`".
* Plenty of existing code doesn't follow this convention; even if everyone followed convention from this point forward, you still cannot make the assumption that `T*` always means "an optional view of `T`".
* Low-level code (new and existing) necessarily uses `T*` to mean all sorts of things; even if all new and old code followed the convention where appropriate, there would still be code where `T*` does not mean "an optional view of `T`", so you _still_ cannot make that assumption.

Conversely, wherever you see `optional_view<T>` in some code, you _know_ that it means "an optional view of `T`" (unless the author of the code was being perverse). Explicitly documenting intent is generally better than letting readers of your code make assumptions.

Aside from the case for documenting intent, it can be argued that pointers and references are not optimally designed for use as view types. A well-designed type is _efficient_ and has an API that is _minimal_ yet _expressive_ and _hard to use incorrectly_. Pointers as view types may be efficient and do have a somewhat expressive API, but their API is not minimal and is very easy to use incorrectly. Case in point, this is syntactically correct but semantically incorrect code:

```c++
int i = 0;
int* p = &i;
p += 1; // did I mean `*p += 1`?
*p = 42; // undefined behaviour
```

On the other hand, references as view types are efficient and have a minimal API that is generally hard to use incorrectly. However, the reference API is not as expressive as it could be, since references cannot be reassigned after construction to reference a different object:

```c++
int i = 0, j = 42;
int& r = i;
r = j; // modifies `i`; does not make `r` reference `j`
```

As a bonus, `view<T>` is compatible with `std::experimental::propagate_const`, while `T&` is not.

`view` and `optional_view` are _as_ efficient as pointers and references, and have been _purpose-designed_ as view types to be minimal, expressive and hard to use incorrectly.

### <a name="faq-operator-dot"></a>Why does `view` use pointer-like syntax when it can't be null? Typing `*` or `->` is a hassle. Shouldn't you wait until some form of `operator.` overloading has been standardized?

Even with some form of `operator.` overloading, `view` would have the same design.

The various `operator.` "overloading" proposals put forward possible C++ language features that could allow one type to "inherit" the API of another type _without_ using traditional inheritance. Such mechanisms _could_ be used to implement `view` such that instead of writing `(*v).bar` (or `v->bar`), where `v` is of type `view<foo>` and `foo` has a member `bar`, we would instead write `v.bar`. At first glance, this makes sense: since a `view` cannot be null, why not model it after a reference instead of a pointer?

Alas, there is no such thing as a free lunch. The question is, what should `view::operator=(T& t)` do? There are two possibilities: either the `view` is reassigned to reference the object referenced by `t`, or the `view` indirectly assigns the object referenced by `t` to the object the `view` references.

If our design specifies the former, then we have potentially surprising asymmetrical behaviour:

```c++
f.bar = g.bar; // modifies `f`
f = g; // modifies `f`

v.bar = g.bar; // modifies `f`
v = g; // modifies `v`
```

If our design specifies the latter, then we have _different_ potentially surprising asymmetrical behaviour:

```c++
f = g; // modifies `f`
f = u; // modifies `f`

v = g; // modifies `f`
v = u; // modifies `v`
```

In both cases, there is an "exceptional" case that the programmer has to remember. In addition, these are not behaviours that they are already likely to be familiar with: `view` would have copy/assignment behaviour not exactly like a reference, and not exactly like a pointer, but like a hybrid of the two. What's more, `view` would then behave differently from `optional_view`! The chance of programming error with either design is very high. `view` avoids these problems by borrowing the indirection and copy behaviour used by pointers and pointer-like types:

```c++
f.bar = g.bar; // modifies `f`
f = g; // modifies `f`
f = u; // error: no conversion from `view<foo>` to `foo`
f = *u; // modifies `f`

v.bar = g.bar; // error: `view<foo>` has no member `bar`
v = g; // modifies `v`
v = u; // modifies `v`
v = *u; // modifies `v`

(*v).bar = g.bar; // modifies `f`
(*v) = g; // modifies `f`
(*v) = u; // error: no conversion from `view<foo>` to `foo`
(*v) = *u; // modifies `f`
```

This behaviour is both symmetrical _and_ familiar to experienced programmers: both `view` and `optional_view` are initialized like a reference, and have copy and indirection behaviour like a pointer. This is predictable behaviour that is easy to remember.

Of course, `operator.` "overloading" does have its uses, but this is not one of them.

### <a name="faq-reference-wrapper"></a>Isn't `view` the same as `std::reference_wrapper`?

No. They are very different. In short, `view` has pointer-like indirection semantics, whereas `std::reference_wrapper` has reference-like direct-access semantics.

The key difference between `view` and `reference_wrapper` is in assignment and comparison. While both types behave the same on construction:

```c++
int i = 42;
reference_wrapper<int> r = i; // `r` indirectly references `i`
view<int> v = i; // `v` indirectly references `i`
```

They have different behaviour on assignment:

```c++
int j = 21;
r = j; // `r` still indirectly references `i`; `i` and `j` are now equal
v = j; // `v` now indirectly references `j`; `i` and `j` are not equal
```

And on comparison:

```c++
if (r == i) { … } // true if the object `r` indirectly references is equal to `i`
if (v == i) { … } // true if v indirectly references `i`
```

In other words, in this case `reference_wrapper` is designed to behave like a reference, while `view` is designed to behave more like a pointer. The difference between a reference and a `reference_wrapper` is that copy assigning the latter will rebind it to indirectly reference whatever the other `reference_wrapper` referenced.

```c++
r = std::ref(j); // `r` now indirectly references `j`
v = make_view(i); // `v` now indirectly references `i`
```

This slightly modified behaviour allows `reference_wrapper`, among other things, to be stored in an STL container. `view` can also be stored in an STL container, but just as a stand-alone `view` behaves very differently than a stand-alone `reference_wrapper`, so too do STL containers of these types. Take, for example, the `remove_child` member function from the tutorial:

```c++
private:
    …
    std::vector<view<node>> children;

public:
    …
    void remove_child(view<node> child) {
        children.erase(std::find(children.begin(), children.end(), child));
    }
```

A call to `remove_child` will erase from the container any `view` which indirectly references the same `node` as `child`. Now say we were to use `reference_wrapper` instead of `view`:

```c++
private:
    …
    vector<reference_wrapper<node>> children;

public:
    …
    void remove_child(node& child) {
        children.erase(std::find(children.begin(), children.end(), child));
    }
```

If `operator==` isn't defined for `node`, then this code won't compile. If we _do_ define `operator==` for `node`, then this code will compile but won't have the same effect as before. A call to `remove_child` will now erase from the container any `view` which indirectly references a `node` which _is equal_ to the `node` indirectly referenced by `child`.

In short, operations on `view` tend to modify or inspect the `view` itself (like a pointer), while operations on `reference_wrapper` tend to modify or inspect the indirectly referenced object (like a reference). There are times when `view` is appropriate and times where `reference_wrapper` is what you need, but they are certainly not interchangeable.

### <a name="faq-not-null"></a>Isn't `view` the same as `gsl::not_null`?

No. The aims of `view` and `not_null` are different. In short, `view<T>` is a view of a single object of type `T`, while `not_null<T*>` is simply a `T*` that should not be null.

According to the [current documentation](https://github.com/isocpp/CppCoreGuidelines/blob/master/CppCoreGuidelines.md#i12-declare-a-pointer-that-must-not-be-null-as-not_null) in the C++ Core Guidelines, the purpose of `not_null` is simply to indicate to the compiler and the reader that a pointer should not be null. The current design of `not_null` allows optional run-time checks to be enabled via a pre-processor switch, but the C++ Core Guidelines suggest enforcement via a static analysis tool. The examples given include use with C-style strings (`char const*`), something that would never be a use case for `view`.

One possible source of confusion is the fact that the [current implementation](https://github.com/Microsoft/GSL/blob/master/gsl/gsl) of `not_null` explicitly `delete`s a number of pointer arithmetic operations. At first, it may seem like an indication that `not_null` should not be used to store pointers that represent arrays or iterators. However, one of the authors of `not_null` has [clarified](https://github.com/Microsoft/GSL/issues/417) that these restrictions simply aim to encourage the use of the complementary `span` facility (the GSL version of an `array_view`) instead of manually iterating over ranges or indexing into arrays represented by pointers.

So `view` and `not_null` are not the same at all. They perform complementary functions and could be used side-by-side in the same codebase, no problem.

### <a name="faq-observer-ptr"></a>Isn't `optional_view` the same as `std::experimental::observer_ptr`?

Both `optional_view` and [`std::experimental::observer_ptr`](http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2014/n4282.pdf) have essentially the same purpose (high-level view/observer types that document intent) but slightly different designs. While the design of `observer_ptr` is based on the existing standard library smart pointer types, `optional_view` is specifically designed to be a view type. The fact that pointers are already _okay_ as view types means that there isn't a great deal of difference between `optional_view` and `observer_ptr`; however, there are a couple of things worth noting.

First, `observer_ptr` has to be explicitly constructed from a pointer:

```c++
foo f, g;
observer_ptr<foo> p{&f};
p = make_observer(&g);
assert(p == make_observer(&g));
p->bar();
p = {};
```

On the other hand, while `optional_view` too allows explicit construction from a pointer, it also allows _implicit_ construction from a _reference_. This tends to mean using `optional_view` feels slightly more natural to use and results in slightly less verbose code than `observer_ptr`:

```c++
foo f, g;
optional_view<foo> v = f;
v = g;
assert(v == g);
v->bar();
v = {};
```

Second, `observer_ptr` lacks a "not null" counterpart. While the case for a so-called "vocabulary" type to replace references is weaker than that for non-owning pointers (`T&` pretty much exclusively means "view of `T`"), the irregular copying behaviour of references makes the case for a complementary "not null" non-owning reference type fairly strong.

There are a number of other differences between `optional_view` and `observer_ptr`, but they are less significant:

* `observer_ptr` has the "ownership" operations `reset` and `release`, presumably because it its API is modelled on the owning smart pointer types.
* `optional_view` has "safe" construction from `T*` which throws if called with a null `T*`.
* `optional_view` has the "safe" accessor function `value` which throws if called on an empty `optional_view`. This part of the API is modelled after `std::optional`.
* `optional_view` has cast operations `static_view_cast`, `dynamic_view_cast` and `const_view_cast`.

The question is, is there room for both `optional_view` (and `view`) _and_ `observer_ptr` in the C++ programmer's toolkit? If a separate case can be made for an "owning smart pointer"-like non-owning "smart" pointer, then perhaps there is. Otherwise, maybe we have two different designs for two competing types trying to solve the same problem. The case for `optional_view` and `view` has been laid out here, but the best design for such a type is of course up for discussion.

### I use `T&` to represent permenant relationships and `T*` for relationships that can be changed. Don't I lose this functionality with `view` and `optional_view`?

The `const` mechanism is the natural way to model permenancy in C++. `view` and `optional_view` provide a natural way to model mandatory and optional relationships. Combining these gives greater flexibility than `T&` or `T*` can alone:

* `view<T>` – a mandatory, changeable relationship
* `view<T> const` – a mandatory, permanent relationship (equivalent to `T&`)
* `optional_view<T>` – an optional, changeable relationship (equivalent to `T*`)
* `optional view<T> const` – an optional, permanent relationship

### Why would you ever use `view<T>` instead of `T&` in an API?

Using `optional_view<T>` in an API has a number of clear advantages over `T*`, including:

* Documentation of intent
* Omission of inappropriate operations
* Natural initialization syntax

The advantages of `view<T>` over `T&` in an API, on the other hand, are not as clear cut. However, they are not always interchangeable.

When used as a function parameter type, `view<T const>` and `T const&` have subtly different behaviour:

```c++
void foo(int const& v) { … }
void bar(view<int const> v) { … }

foo(42);
foo({}); // same as `foo(int{})`, i.e. `foo(0)`

bar(42);
bar({}); // error: cannot convert from `{}` to `view<int const>`
```

The minor advantage of using `view` here is that client code won't break if you switch to `optional_view` (with which `{}` represents the "empty" state) at some point in the future. Other than this, there is little difference between the two. When used as a function return type, `view<T>` and `T&` are more obviously different:

```c++
int& foo() { … }

int i = 42;
foo() = i; // assigns 42 to whatever `foo` returned a reference to
auto x = foo(); // `decltype(x)` is `int`
```

```c++
view<int> foo() { … }

int i = 42;
foo() = i; // modifies the temporary `view` returned by `foo` to reference `i`
auto x = foo(); // `decltype(x)` is `view<int>`
```

`view<T>` has pointer-like indirection semantics, while `T&` has reference-like direct-access semantics. Thus, you would not want to use `view` when you wished to provide _direct_ access to an object, like when implementing `operator[]` for an array type. You _may_ want to use `view` if you wish to provide a pointer-like indirect handle to an object; in this case you may also wish to return a `view<T> const` to prevent the caller from accidentally assigning to the temporary:

```c++
view<int> const foo() { … }

int i = 42;
foo() = i; // error: cannot call `operator=` on `view<int> const`
```

The exact philosophical difference between `T&` and `view<T>` is up for discussion.
