## `view` and `optional_view`: indirection types for C++

`view` and `optional_view` are types used to indirectly reference other objects without implying ownership. They offer a number of advantages over raw pointers and references, including:

* Increased safety
* Clearer intent
* A more natural API 

### Quick Example

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

A mandatory relationship can be expressed by using `view<T>` instead of `optional_view<T>`:

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
auto dolly = *bob.pet;
```

### Tutorial

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

### Design Rationale

#### Conversion to and from `T*` and `T&`

#### Named member functions (`has_value`, `value` and `value_or`)

#### Conversion from `view` to `bool`

#### Naming

### FAQ

#### Aren't `T*` and `T&` good enough?

The suggestion has been made that raw pointers work just fine as non-owning, optional reference types, and there is no need to introduce new types when we already have long-established types that are well-suited to this role.

Some argue that once all other uses of pointers have been covered by other high-level types (e.g. `std::unique_ptr`, `std::string`, `std::string_view`, `std::array`, `std::array_view`, …) the only use case that will be left for raw pointers will be as non-owning, optional reference types, so it will be safe to assume that wherever you see `T*` it means "non-owning, optional reference". This is not true for a number of reasons:

* This is only a convention; people are not obliged to use `T*` only to mean "non-owning, optional reference".
* Plenty of existing code doesn't follow this convention; even if everyone followed convention from this point forward, you still cannot make the assumption that `T*` always means "non-owning, optional reference".
* Low-level code (new and existing) necessarily uses `T*` to mean all sorts of things; even if all new and old code followed the convention where appropriate, there would still be code where `T*` does not mean "non-owning, optional reference", so you _still_ cannot make that assumption.

Conversely, wherever you see `optional_view<T>` in some code, you _know_ that it means "non-owning, optional reference" (unless the author of the code was being perverse). Explicitly documenting intent is generally better than letting readers of your code make assumptions.

Aside from the case for documenting intent, it can be argued that pointers and references are not optimally designed for use as non-owning reference types. A well-designed type is efficient and has an API that is minimal yet expressive and hard to use incorrectly. Pointers as optional, mandatory reference types may be efficient and have an expressive API, but their API is not minimal and is very easy to use incorrectly. Case in point, this is syntactically correct but semantically incorrect code:

```c++
int i = 0;
int* p = &i;
p += 1;
*p = 42; // undefined behaviour
```

On the other hand, references as non-owning, mandatory reference types are efficient and have a minimal API that is generally hard to use incorrectly. However, the references API is not as expressive as it could be, since references cannot be reassigned to refer to a different object after construction:

```c++
int i = 0, j = 42;
int& r = i;
r = j; // modifies `i`; does not make `r` reference `j`
```

To overcome this lack of functionality, we must instead use pointers, which means we have to account for the null state, or use something like `std::reference_wrapper`, which still has reference-like behaviour and is unlikely to be what we want.

`view` and `optional_view` fix all of these problems since they have been designed for-purpose.

#### Why does `view` use pointer-like syntax when it can't be null? Typing `*` or `->` is a hassle. Shouldn't you wait until some form of `operator.` overloading has been standardized?

Even with some form of `operator.` overloading, `view` would have the same design.

#### Isn't `view` the same as `std::reference_wrapper`?

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

#### Isn't `view` the same as `gsl::not_null`?

It depends. The design goals of `gsl::not_null` seem to be in flux.

As currently documented, `not_null` claims to be usable with owning smart pointer types, like `std::unique_ptr` and `std::shared_ptr`, as well as with raw pointers. `view`, on the other hand, is explicitly _non-owning_. This difference of design is reflected in the interface of each type; with `view<T>`, `T` is the type of object it indirectly references, while with `not_null<T>`, `T` is the pointer-like type it adapts:

```c++
not_null<int*> n = i;
view<int> v = i;
```

However, removing the null state from owning smart pointers is without its consequences. For example, a moved-from `unique_ptr` is necessarily null, since ownership of the managed object has been transferred. This means that a "not null" `unique_ptr` is neither copyable _nor_ movable, which arguably limits its usefulness. This issue has been recognized by the GSL project in a number of different issues, and it seem that current efforts are focused on the raw pointer use case. If support for smart pointers were dropped in the future, it would arguably make sense for `not_null<T>` to take the type of object it references, just like `view<T>`:

```c++
not_null<int> n = i;
```

At this point, the difference between `view` and `not_null` would be less clear. The next biggest difference between `view` and `not_null` is probably the existence of `optional_view`. At one point, there existed a `maybe_null` counterpart to `not_null`. However, this [was removed](https://github.com/isocpp/CppCoreGuidelines/issues/271) because it _"makes code verbose"_. As far as I am aware, the designers of `not_null` are of the opinion that

Even if 

#### Isn't `optional_view` the same as `std::experimental::observer_ptr`?

#### Couldn't the job of `view` and `optional_view` be done by `gsl::not_null` and `std::experimental::observer_ptr`?

