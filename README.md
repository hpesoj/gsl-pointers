# `view` and `optional_view`: reference types for C++

`view<T>` and `optional_view<T>` are wrappers intended to replace `T&` and `T*` wherever they are used as non-owning references.

## Tutorial

As a demonstration we will create a type which can be used to create a tree of connected nodes. Each node will have zero or one parent and zero or more children. For simplicity, we will not attempt to prevent cycles from being formed.

With only references and pointers in your arsenal, you might start with something like this:

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

Using a pointer to for the parent works well because `nullptr` can be used to indicate the lack of a parent. However, we already have a bug in our code: we forgot to initialize `parent` to `nullptr`! In addition, the meaning of `node*` is not 100% clear. We want it to mean "_non-owning_, _optional_ reference to _single_ `node` object". However, pointers are sometimes used in place of references, where they are assumed to _not_ be null; a violation of this pre-condition is a programming error and may result in undefined behaviour. Pointers are also used to represent arrays of objects; indeed, pointers define the `++` and `--` operations for navigating arrays, but incrementing and dereferencing a pointer which references a single object is undefined behaviour. Occasionally, a function which takes a pointer may assume ownership of the referenced object and call `delete` on the pointer at some point in the future. Needless to say, calling `delete` twice on the same object is bad news.

Now, we could provide additional documentation to tell the user what we mean when we say `node*`, but if we replace `node*` with `optional_view<node>`, we can convey that information automatically. An `optional_view<T>` _is_ a __non-owning__, __optional__ reference to a __single__ object of type `T`. In addition, we get compile-time assurances that we didn't with `T*`:

* `optional_view<T>` always default initializes to null
* `optional_view<T>` cannot be incremented or decremented
* `optional_view<T>` cannot be `delete`d.

What's more, `optional_view<T>` implicitly converts to and from `T*`, so most client code should be completely unaffected. Let's replace `node*` with `optional_view<node>`:

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

An additional nicety is that `optional_view<T>` can be implicitly constructed from `T&` (note: it will not implicitly convert _to_ `T&` for reasons of safety). This means that we can use our `node` class like this:

```c++
node a, b;
b.set_parent(a);
```

Of course, we could have overloaded `set_parent` to take both `node*` _and_ `node&`, but who wants to do the extra work, or indeed, can remember to do it consistently for all APIs?

Now, it would be nice if `node` kept track of its children, so we can navigate both up _and_ down the tree. If we were using references and pointers, we might initially think of adding something like this:

```c++
    std::vector<node&> children;
```

Alas, this will not compile. References behave unusually: unlike pointers, copy assigning to a reference will modify the referenced object, not the reference itself. This behaviour means they cannot be stored in STL containers like `std::vector`. Instead, we are forced to store pointers:

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

This works, but it's a red flag that we are using a pointer because we _have_ to, rather than because it makes sense. Using a pointer for the node's parent makes sense, because `nullptr` can be used to represent the "no parent" state, but "no children" is simply indicated by an empty vector. There is no reason for a child to be null; indeed, it would probably be a bug if a child were to somehow become null.

The solution is `view`, the non-optional counterpart to `optional_view`. In addition to all the documentation benefits and compile-time assurances that `optional_view` provides, `view` gives us the assurance that it will never, and _can_ never, be null. An `optional_view<T>` _is_ a __non-owning__, ___mandatory___ reference to a __single__ object of type `T`. And it can be copied and stored in a container, just like `optional_view<T>` or `T*`.

Let's replace all `node*` and `node&` with `view<node>`, and add the calls to `add_child` and `remove_child` to `set_parent`:

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

As well as the implementation being simpler and safer (no calls to `operator&` and `operator*`), the API has become more consistent and harder to misuse. Consider this example:

```c++
node a, b;
b.set_parent(a);

auto a0 = b.get_parent();
auto b0 = a.get_child(0);
```

What do `a0` and `b0` contain? With the implementation using `view` and `optional_view`, `a0` is of type `optional_view<node>` and `b0` is of type `view<node>`. With the implementation using references and pointers, `a0` is of type `node*` and `b0` is of type… `node`. Due to the `auto` type deduction rules, `a0` is a copy of `a`, not a reference to `a`. This may not be what we intended. To prevent a copy, you must state explicitly that you want a reference:

```c++
auto a0 = b.get_parent();
auto& b0 = a.get_child(0);
```

Conversely, with `view` and `optional_view`, you must explicitly state that you want to allow copying by using either `operator*` or `value`:

```c++
auto a0 = b.get_parent().value(); // throws if optional_view<T> is null
auto b0 = *a.get_child(0); // is safe for view<T>
```

It's worth noting that it is probably impossible to define sensible copy behaviour for our `node` class, since each node can have only one parent, so we would probably want to delete the copy constructor and assignment operator. The example was chosen for simplicity as well as to showcase `optional_node`; it _would_ be possible to have sensible copy behaviour if each node could have multiple parents, so the benefit of explicit copy syntax _is_ real.

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
