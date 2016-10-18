# `view` and `optional_view`: reference types for C++

`view<T>` and `optional_view<T>` are wrappers intended to replace `T&` and `T*` wherever they are used as non-owning references.

## Tutorial

For this demonstration, we will create a type that can be used to create a tree of connected nodes. Nodes will not own other nodes, but will instead keep non-owning references to other nodes. Each node will have zero or one parent and zero or more children. For simplicity, we will not attempt to prevent cycles from being formed.

With only references and pointers in our arsenal, we might start with something like this:

```c++
class node {
private:
    node* parent;
    
public:
    void set_parent(node* new_parent) {
        parent = new_parent;
    }
    
    node const* get_parent() const {
        return parent;
    }

    node* get_parent() {
        return parent;
    }
};
```

We make `parent` a pointer because it is natural to use `nullptr` to represent the lack of a parent. However, we already have a bug in our code: we forgot to initialize `parent` to `nullptr`! In addition, the meaning of `node*` is not 100% clear. We want it to mean "_non-owning_, _optional_ reference to a _single_ `node` object". However, pointers have other potential uses:

* Pointers are sometimes used in place of references, where they are assumed to _not_ be null; a violation of this condition is a programming error and may result in undefined behaviour.
* Pointers are used to represent arrays of objects; indeed, pointers define arithmetic operations (e.g. `++` and `--`) operations for navigating arrays, but modifying and then dereferencing a pointer which references a single object is undefined behaviour.
* Occasionally, ownership of a dynamically created object (using keyword `new` or otherwise) may be transferred when calling a function that receives or returns a pointer; that is, the creator of the object is no longer responsible for destroying it (using keyword `delete` or otherwise). Neglecting to destroy an object you owner will result in a resource leak. Attempting to destroying an object you don't own can result in underfined behaviour.

With all this potential undefined behaviour, it is important that a client of our API knows exactly what `node*` _means_. We could provide additional documentation to tell the user what we mean when we say `node*`, but if we replace `node*` with `optional_view<node>`, we can convey that information automatically. An `optional_view<T>` _is_ a __non-owning__, __optional__ reference to a __single__ object of type `T`. In addition, we get compile-time assurances that we didn't with `T*`:

* `optional_view<T>` is always default initialized
* `optional_view<T>` does not define arithmetic operations
* `optional_view<T>` does not implicitly convert to `void*` (you cannot `delete` it)

What's more, `optional_view<T>` implicitly converts to and from `T*`, so most client code should be completely unchanged. Let's replace `node*` with `optional_view<node>`:

```c++
class node {
private:
    optional_view<node> parent;
    
public:
    void set_parent(optional_view<node> new_parent) {
        parent = new_parent;
    }
    
    optional_view<node const> get_parent() const {
        return parent;
    }

    optional_view<node> get_parent() {
        return parent;
    }
};
```

An additional nicety is that `optional_view<T>` can be implicitly constructed from `T&` (note: it will not implicitly convert _to_ `T&`, for reasons of safety). This means that we call `set_parent` without having to take the address of the new parent:

```c++
node a, b;
b.set_parent(a);
```

Not only this the calling syntax nicer, but it discourages the use of raw pointers in client code, reducing the potential for `nullptr` bugs. Of course, we could have overloaded `set_parent` to take both `node*` _and_ `node&`, but who wants to do the extra work, or indeed, can remember to do it consistently for all APIs?

Now, it would be nice if `node` kept track of its children, so we can navigate both up _and_ down the tree. If we were using references and pointers, we might initially consider storing a `std::vector` of references:

```c++
    std::vector<node&> children;
```

Alas, this will not compile. References behave unusually: unlike pointers, copy assigning to a reference will modify the referenced object, not the reference itself. This behaviour precludes storage of references in STL containers like `std::vector`. Instead, we are forced to store pointers:

```c++
private:
    …
    std::vector<node*> children;
    
public:
    …
    
    std::size_t get_child_count() const {
        return children.size();
    }

    node& get_child(std::size_t index) {
        return *children[index];
    }

    node const& get_child(std::size_t index) const {
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

This works, but the fact that we are using a pointer because we _have_ to, rather than because it makes sense, should set alarm bells ringing. Using a pointer for `parent` makes sense, because `nullptr` can represent the "no parent" state, but "no children" is indicated by an empty vector. There is no reason for a child to be null; indeed, it would probably be a bug if a child _were_ to somehow become null.

In addition to being unable to use store references in STL containers, the meaning of `node const&` (and to a lesser extent, `node&`) is not 100% clear either. We want it to mean "_non-owning_, _mandatory_ reference to a _single_ `node` object", but references to const are often used as an optimization technique, to avoid making unnecessary copies. Consider this function signature:

```c++
    node const& get_child(std::size_t index) const;
```

Does the use of `node const&` mean we should be storing [TODO: complete]

Instead of `node&` or `node*`, we can use `view<node>`, the non-optional counterpart to `optional_view<node>`. In addition to all the documentation benefits and compile-time assurances that `optional_view` provides, `view` _must_ always reference an object. An `optional_view<T>` _is_ a __non-owning__, ___mandatory___ reference to a __single__ object of type `T`. And it can be copied and stored in a container, just like `optional_view<T>` or `T*`.

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

    view<node> get_child(std::size_t index) {
        return children[index];
    }

    view<node const> get_child(std::size_t index) const {
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

As well as the implementation being simpler and safer (no use of `&` or `*` – except for `*this`, but `this` probably [should have been a reference](http://www.stroustrup.com/bs_faq2.html#this) anyway), the API has become more consistent and harder to misuse. Consider this example:

```c++
node a, b;
b.set_parent(a);

auto a0 = b.get_parent();
auto b0 = a.get_child(0);
```

What are the types of `a0` and `b0`? With the implementation using `view` and `optional_view`, the answer is simple: `a0` is of type `optional_view<node>` and `b0` is of type `view<node>`. On the other hand, when using references and pointers, `a0` is of type `node*` and `b0` is of type… `node`. Due to the `auto` type deduction rules, `b0` is in fact a copy of `b`. In our case, this is unlikely to have been intentional. To prevent a copy, you must state explicitly that you want a reference:

```c++
auto a0 = b.get_parent();
auto& b0 = a.get_child(0);
```

Conversely, with `view`, the referenced object will not be copied, even if you explicitly specify the type of the target object:

```c++
node c = a.get_child(0); // error: attempt to call deleted function
```

To copy the referenced object, you must explicitly access it by "dereferencing" the `view`:

```c++
auto c = *a.get_child(0);
```

To copy the value of an `optional_view`, you should either ensure the `optional_view` is not null before "dereferencing" it, or call `value`, which will throw if the `optional_view` is null:

```c++
if (b) {
    auto d = *b.get_parent();
}

auto e = b.get_parent().value();
```

It's worth noting that it is probably impossible to define sensible copy behaviour for our `node` class, since each node can have only one parent, so we would probably want to delete the copy constructor and assignment operator. The example was chosen for simplicity as well as to showcase `optional_node`; it _would_ be possible to have sensible copy behaviour if each node could have multiple parents, so the benefit of explicit copy syntax is not just theoretical.

## FAQ

### Isn't `view` practically the same as `std::reference_wrapper`?

Both types perform a similar function but have very different behaviour.

Say we have a type, `foo`, which contains a single integer and provides an equality comparison operator:

```c++
struct foo {
  int bar = 0;
};

bool operator==(foo const& lhs, foo const& rhs) {
  return lhs.bar == rhs.bar;
}
```

Now imagine we have the following code:

```c++
foo a, b, c;
std::vector<foo*> refs = { &a, &b, &c };
erase_first(refs, &b));
```

The implementation of `erase_first` looks like this:

```c++
template <typename Container, typename T>
void erase_first(Container& c, T const& value) {
    c.erase(std::find(std::begin(c), std::end(c), value));
}
```

So we create a vector of pointers to instances of `foo` then erase the first (and only in this case) instance of `b`. But perhaps we don't like the idea of a null pointer slipping into our vector: we want strong assurance that this _cannot_ happen. Perhaps we can use a vector of `std::reference_wrapper<foo>` instead of `foo*`?

```c++
std::vector<std::reference_wrapper<foo>> refs = { a, b, c };
erase_first(refs, b));
```

Great, it compiles! But unfortunately, the behaviour of our code has changed: the reference to `a`, not `b`, is erased. This is because `operator==` is not defined for `std::reference_wrapper<T>`, but it does implicitly convert to `T&`: the call to `std::find` is comparing the referenced `foo` objects rather than the reference wrappers themselves; since `a` is equal to `b` and comes before it in the vector, `a` is erased.

This behaviour is intentional: `std::reference_wrapper` mimics the behaviour of C++ references, but allows them to be copy assigned, and therefore stored in containers. We _can_ get the behaviour we want by replacing the call to `erase_first` with a call to `erase_first_ref`, a function which explicitly compares addresses instead of values:

```c++
template <typename Container, typename T>
void erase_first_ref(Container& c, T const& value) {
    auto it = std::find_if(std::begin(c), std::end(c), [&value](T const& x) {
        return &x == &value;
    });

    if (it != std::end(c)) {
        c.erase(it);
    }
}
```

But a better solution is to use a vector of `view<foo>`:

```c++
std::vector<view<foo>> refs = { a, b, c };
erase_first(refs, b);
```

This just works, because `operator==` is defined for comparison of `view<T>` and `T const&`, and compares the addresses of the referenced objects. Note that it is important that `erase_first` take its second argument by reference, or the address compared will be that of a local variable (and `erase_first` would never erase anything).

The value of `view<T>` is defined by the address of the referenced object. All implicit operations on a `view` refer to the `view` itself rather than the referenced object; a `view` must be explicitly "dereferenced" to access the value of the referenced object. Even conversion from `view<T>` to `T` is prohibited, though implicit conversion from `view<T>` to `T&` _is_ allowed. In this way, `view<T>` behaves like a `T*` that cannot be null.

In summary, `std::reference_wrapper` is a transparent reference wrapper whose behaviour mimics that of references as closely as possible. Conversely, `view` is a reference wrapper whose behaviour more closely mimics that of pointers, but with conversion from and comparison to references.
