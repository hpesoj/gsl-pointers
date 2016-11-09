# `view` and `optional_view`: indirection types for C++

`view` and `optional_view` are types used to indirectly reference other objects without implying ownership. They offer a number of advantages over raw pointers and references, including:

* Increased safety
* Clearer intent
* A more natural API 

## Quick Example

Take an example where a pointer might be used to represent an optional relationship between a person and their pet animal:

```c++
struct person {
    animal* pet = nullptr;
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
bob.pet = fluffy;
```

The "empty" status of an `optional_view` can be tested for as if it were a `bool`, and the `optional_view` can be "emptied" by assigning `nullopt`:

```c++
if (bob.pet)
    bob.pet = nullopt;
```

A mandatory relationship can be expressed by using `view` instead of `optional_view`:

```c++
struct person {
    view<animal> pet;
};

animal fluffy;
person bob{fluffy}; 
```

Like pointers, both `view` and `optional_view` use the indirection operators (`*` and `->`) to access the referenced object:

```c++
bob.pet->sleep();
animal dolly = *bob.pet;
```

Both `view` and `optional_view` can be copied freely, and `view` implicitly converts to `optional_view`:

```c++
view<animal> pet = bob.pet;
optional_view<animal> maybe_pet = pet;
```

## Tutorial

For this demonstration, we will create a type that can be used to create a tree of connected nodes. Nodes will not own other nodes, but will instead keep non-owning references to other nodes. Each node will have zero or one parent and zero or more children. For simplicity, we will not attempt to prevent cycles from being formed.

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

We make `parent` a pointer because it is natural to use `nullptr` to represent the lack of a parent. However, we already have a bug in our code: we forgot to initialize `parent` to `nullptr`! In addition, the meaning of `node*` is not 100% clear. We want it to mean "_non-owning_, _optional_ reference to a _single_ `node` object". However, pointers have other potential uses:

* Pointers are sometimes used in place of references, where they are assumed to _not_ be null; a violation of this condition is a programming error and may result in undefined behaviour.
* Pointers are used to represent arrays of objects; indeed, pointers define arithmetic operations (e.g. `++` and `--`) operations for navigating arrays, but modifying and then dereferencing a pointer which references a single object is undefined behaviour.
* Occasionally, ownership of a dynamically created object (using keyword `new` or otherwise) may be transferred when calling a function that receives or returns a pointer; that is, the creator of the object is no longer responsible for destroying it (using keyword `delete` or otherwise). Neglecting to destroy an object you owner will result in a resource leak. Attempting to destroying an object you don't own can result in underfined behaviour.

With all this potential undefined behaviour, it is important that a client of our API knows exactly what `node*` _means_. We could provide additional documentation to tell the user what we mean when we say `node*`, but if we replace `node*` with `optional_view<node>`, we can convey that information automatically. An `optional_view<T>` _is_ a __non-owning__, __optional__ reference to a __single__ object of type `T`. In addition, we get compile-time assurances that we didn't with `T*`:

* `optional_view<T>` is always default initialized (to its empty state)
* `optional_view<T>` does not define arithmetic operations
* `optional_view<T>` does not implicitly convert to `void*` (you cannot `delete` it)

Let's replace `node*` with `optional_view<node>`:

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

Note that while `optional_view<T>` can be _explicitly_ constructed from `T*`, `optional_view<T>` is implicitly constructible from `T&`. This means that we call `set_parent` and test the result of a call to `get_parent` without taking the address of the other `node` object:

```c++
node a, b;
b.set_parent(a)
assert(b.get_parent() == a);
```

Not only is the syntax nicer, but it discourages the use of raw pointers in client code, reducing the potential for `nullptr` bugs. To set the empty state, we use `nullopt`, and to test for the empty state we can test `optional_view` as if it were a `bool`:

```c++
b.set_parent(nullopt)
assert(!b.get_parent());
```

Now, it would be nice if `node` kept track of its children, so we can navigate both up _and_ down the tree. If we were using references and pointers, we might initially consider storing a `std::vector` of references:

```c++
    std::vector<node&> children;
```

Alas, this will not compile. References behave irregularly: unlike pointers, copy assigning to a reference will modify the referenced object, not the reference itself. This behaviour precludes storage of references in STL containers like `std::vector`. Instead, we are forced to store pointers:

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

Instead of `node&` or `node*`, we can use `view<node>`, the mandatory counterpart to `optional_view<node>`. In addition to all the documentation benefits and compile-time assurances that `optional_view` provides, `view` _must_ always reference an object. An `optional_view<T>` _is_ a __non-owning__, ___mandatory___ reference to a __single__ object of type `T`. And it can be stored in STL containers, just like `optional_view<T>` or `T*`.

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
    
    std::size_t get_child_count() const {
        return children.size();
    }

    view<node> get_child(std::size_t index) const {
        return copy_view(children[index]);
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

As well as the implementation being safer, the way the API is used has become more consistent. Previously, different syntax would have been required to store and access `node&` and `node*`:

```c++
auto aa = b.get_parent();
auto& bb = a.get_child(0); // omitting the `&` would perform a copy

bb.set_parent(nullptr);
aa->set_parent(&bb);
```

With `view<node>` and `optional_view<node>`, the syntax is more consistent:

```c++
auto aa = b.get_parent();
auto bb = a.get_child(0);

bb->set_parent(nullptr);
aa->set_parent(bb);
```

Of course, it is subjective as to whether or not this is an improvement; the requirement to use `->` instead of `.` where `view<T>` replaces `T&` could also be considered a syntactic burden. It is always possible to use `view<T>` and `operator_view<T>` only in your implementation, while using `T&` and `T*` in your API.

## Design Rationale

### <a name="rationale-conversion-from"></a>Conversion from `T&` and `T*` to `view` and `optional_view`

Both `view` and `optional_view` are implicitly constructible and assignable from `T&`, while only `optional_view` is explicitly constructible from `T*`. This design decision is based on the idea that implicit conversions should be semantically correct practically all of the time, while explicit conversions are semantically correct only some of the time. `T&` always represents 

### <a name="rationale-conversion-to"></a>Conversion from `view` and `optional_view` to `T&` and `T*`

### <a name="rationale-get"></a>The `get` member function

### <a name="rationale-view-bool"></a>Conversion from `view` to `bool`

### <a name="rationale-nullopt"></a>Use of `nullopt`

### <a name="rationale-named-member-functions"></a>Named member functions (`has_value`, `value` and `value_or`)

### <a name="rationale-naming"></a>Naming

## FAQ

### <a name="faq-good-enough"></a>Aren't `T*` and `T&` good enough?

The suggestion has been made that raw pointers work just fine as non-owning, optional reference types, and there is no need to introduce new types when we already have long-established types that are well-suited to this role.

Some argue that once all other uses of pointers have been covered by other high-level types (e.g. `std::unique_ptr`, `std::string`, `std::string_view`, `std::array`, `std::array_view`, …) the only use case that will be left for raw pointers will be as non-owning, optional reference types, so it will be safe to assume that wherever you see `T*` it means "non-owning, optional reference". This is not true for a number of reasons:

* This is only a convention; people are not obliged to use `T*` only to mean "non-owning, optional reference".
* Plenty of existing code doesn't follow this convention; even if everyone followed convention from this point forward, you still cannot make the assumption that `T*` always means "non-owning, optional reference".
* Low-level code (new and existing) necessarily uses `T*` to mean all sorts of things; even if all new and old code followed the convention where appropriate, there would still be code where `T*` does not mean "non-owning, optional reference", so you _still_ cannot make that assumption.

Conversely, wherever you see `optional_view<T>` in some code, you _know_ that it means "non-owning, optional reference" (unless the author of the code was being perverse). Explicitly documenting intent is generally better than letting readers of your code make assumptions.

Aside from the case for documenting intent, it can be argued that pointers and references are not optimally designed for use as non-owning reference types. A well-designed type is __efficient__ and has an API that is __minimal__ yet __expressive__ and __hard to use incorrectly__. Pointers as optional, mandatory reference types may be efficient and do have an expressive API, but their API is not minimal and is very easy to use incorrectly. Case in point, this is syntactically correct but semantically incorrect code:

```c++
int i = 0;
int* p = &i;
p += 1;
*p = 42; // undefined behaviour
```

On the other hand, references as non-owning, mandatory reference types are efficient and have a minimal API that is generally hard to use incorrectly. However, the reference API is not as expressive as it could be, since references cannot be reassigned to refer to a different object after construction:

```c++
int i = 0, j = 42;
int& r = i;
r = j; // modifies `i`; does not make `r` reference `j`
```

`view` and `optional_view` are _as_ efficient as pointers and references, and have APIs that have been _purpose-designed_ as non-owning reference types to be minimal, expressive and hard to use incorrectly.

### <a name="faq-operator-dot"></a>Why does `view` use pointer-like syntax when it can't be null? Typing `*` or `->` is a hassle. Shouldn't you wait until some form of `operator.` overloading has been standardized?

Even with some form of `operator.` overloading, `view` would have the same design.

The various `operator.` "overloading" proposals present C++ language features that could be introduced to allow one type to "inherit" the API of another type without using traditional inheritance. Such mechanisms _could_ be used to implement `view` such that instead of writing `(*v).bar` (or `v->bar`), where `v` is of type `view<foo>` and `foo` has a member `bar`, we would instead write `v.bar`. At first glance, this makes sense: since a `view` cannot be null, why not model it after a reference instead of a pointer?

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

In both cases, there is an "exceptional" case that the programmer has to remember. These are also not behaviours that they are already likely to be familiar with. The chance of programming error with either design is high. Thankfully, `view` avoids these problems by leveraging the "indirection" semantics used by pointers and pointer-like types:

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

This behaviour is both symmetrical _and_ familiar to experienced programmers. Of course, `operator.` "overloading" does have uses, but this is not one of them.

### <a name="faq-reference-wrapper"></a>Isn't `view` the same as `std::reference_wrapper`?

No. They are very different.

The key difference between `view` and `std::reference_wrapper` is in assignment and comparison. While both types behave the same on construction:

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

In other words, `reference_wrapper` is designed to behave like a reference, while `view` is designed to behave more like a pointer. The difference between a reference and a `reference_wrapper` is that copy assigning the latter will rebind it to indirectly reference whatever the other `reference_wrapper` referenced. 

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

It depends. The design goals of `gsl::not_null` are not 100% clear.

According to the [current documentation](https://github.com/isocpp/CppCoreGuidelines/blob/master/CppCoreGuidelines.md#i12-declare-a-pointer-that-must-not-be-null-as-not_null) in the C++ Core Guidelines, the purpose of `not_null` is simply to indicate to the compiler and the reader that a pointer should not be null. The current design of `not_null` allows optional run-time checks to be enabled via a pre-processor switch, but the C++ Core Guidelines suggest enforcement via a static analysis tool. The examples given include use with C-style strings (`char const*`) so it seems that `not_null` is intended to be a transparent pointer-like adapter that helps ensure the wrapped pointer does not become null. If this is the case, then no, `view` and `not_null` are not the same at all. They perform complementary functions and could be used side-by-side in the same codebase, no problem.

However, the [current implementation](https://github.com/Microsoft/GSL/blob/master/gsl/gsl) contradicts the C++ Core Guidelines, stating that `not_null` should be used only with pointers that reference stand-alone objects (i.e. not arrays of objects), and explicitly `delete`s a number of pointer arithmetic operations. If this is the case, then the function of `not_null` does overlap with `view`. However, I would suggest that `not_null` is not particularly well designed for this purpose, for a few reasons:

* The name `not_null<T*>` simply suggests a pointer that cannot be null; nowhere is it suggested that `not_null<T*>` cannot be used with pointers to arrays; this is misleading.
* There is no `maybe_null` counterpart to `not_null` (`maybe_null` did exist once upon a time, but [was removed](https://github.com/isocpp/CppCoreGuidelines/issues/271) because it "[made] code verbose"); this document clearly outlines the case for `optional_view`; if the designers of `not_null` feel it is important to restrict `not_null` to use with pointers to stand-alone objects (e.g. by omitting pointer arithmetic operations) then they should feel it is important to have a `maybe_null` type to do the same for nullable pointers.
* There are a number of other differences between the designs of `not_null` and `view` that I won't go over here, because it is likely that `not_null` is not intended to perform the same function as `view`.

In short, if `not_null` _is_ intended to reference only stand-alone objects, it is largely redundant in the presence of `view` and `optional_view`, which do a far better job as it stands.

### <a name="faq-observer-ptr"></a>Isn't `optional_view` the same as `std::experimental::observer_ptr`?

Both `optional_view` and [`std::experimental::observer_ptr`](http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2014/n4282.pdf) have essentially the same purpose (high-level non-owning reference types that document intent) but slightly different designs. While `observer_ptr` takes the API from the existing standard library smart pointer types largely unchanged, `optional_view` is intended have whatever API best suits its purpose. The fact that pointers are already _pretty good_ as non-owning reference types means that there isn't a great deal of difference between `optional_view` and `observer_ptr`; however, there are a couple of things worth noting.

First, `observer_ptr` has to be explicitly constructed from a pointer:

```c++
foo f, g;
observer_ptr<foo> p{&f};
p = make_observer(&g);
assert(p == make_observer(&g));
p->bar();
p = nullptr;
```

On the other hand, while `optional_view` too allows explicit construction from a pointer, it also allows _implicit_ construction from a _reference_ (or `nullopt`). This tends to mean using `optional_view` feels slightly more natural to use and slightly less verbose than `observer_ptr`:

```c++
foo f, g;
optional_view<foo> v = f;
v = g;
assert(v == g);
v->bar();
v = nullopt;
```

Second, `observer_ptr` lacks a "not null" counterpart. While the case for a so-called "vocabulary" type to replace references is weaker than that for non-owning pointers (the meaning of `T&` is not as heavily overloaded as `T*`), the irregular copying behaviour of references makes the case for a complementary "not null" non-owning reference type fairly clear. 

There are a number of other differences between `optional_view` and `observer_ptr`, but they are less significant:

* `observer_ptr` has the "ownership" operations `reset` and `release`, presumably because it its API is modelled on the owning smart pointer types.
* `optional_view` has the "safe" accessor function `value` which throws if called on an empty `optional_view`, as well as the named function `has_value` to check for empty status. This part of the API is modelled after `std::optional`.
* `optional_view` has cast operations `static_view_cast`, `dynamic_view_cast` and `const_view_cast`.

The question is, is there room for both `optional_view`/`view` _and_ `observer_ptr` in the C++ programmer's toolkit? If a separate case can be made for an "owning smart pointer"-like non-owning "smart" pointer, then perhaps there is. Otherwise, maybe we have two different designs for two competing types trying to solve the same problem. The case for `optional_view` has been laid out here, but the best design for such a type is of course up for discussion.
