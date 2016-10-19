## `view` and `optional_view`: const-correct references for C++

`view<T>` and `optional_view<T>` are const-correct wrappers intended to replace `T&` and `T*` wherever they are used as non-owning references.

### Tutorial

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
        parent = new_parent; // error: calling deleted copy constructor
    }
    
    optional_view<node> get_parent() const {
        return parent; // error: calling deleted copy constructor
    }
};
```

Oh dear, `optional_view` isn't copyable? Well, not strictly, and for good reason. Unlike `T*`, `optional_view<T>` is _const-correct_: it forwards its "constness" to the object it references. This ensures that, for example, if I have a const reference to a `node` (I cannot modify its value), that I can only acquire a const reference to its parent (so I cannot modify its value either). Of course, you _can_ cast the constness away using `const_view_cast`, but keep in mind this may enable incorrect and unsafe behaviour:

    optional_view<node> get_parent() const {
        return const_view_cast<node>(parent); // here be dragons
    }    

Unfortunately for reference-like types, enforcing const-correctness means that the copy constructor and assignment operator must be deleted, otherwise it would be possible to defeat const-correctness without casting:

```c++
    optional_view<node> get_parent() const {
        return optional_view<node>(parent); // a-okay: copy constructor takes reference to const
    }    
```

Fortunately, there is no reason why `optional_node` should not be copyable in theory; we just can't allow it via the standard C++ copy mechanisms; we have to be _explicit_. The utility function `copy_view` will return a copy of your `optional_view` (or `view`) with the correct constness; if you pass a non-const `optional_view<T>`, it will return an `optional_view<T>`; if you pass a _const_ `optional_view<T>`, it will return an `optional_view<T const>`. Since `optional_view` _is_ movable, the result can be assigned freely.

Let's modify our example so that it compiles:

```c++
    optional_view<node const> get_parent() const {
        return copy_view(parent);
    }

    optional_view<node> get_parent() {
        return copy_view(parent);
    }
```

We provide both const and non-const versions of `get_parent` that return const and non-const views of the parent respectively. Notice that by using `optional_view`, our API has automatically become const-correct itself. This is a really nice feature, the importance of which cannot be overstated!

An additional nicety when using `optional_view` is that `optional_view<T>` can be implicitly constructed from `T&` (note: it will not implicitly convert _to_ `T&`, for reasons of safety) and compared to `T&`. This means that we call `set_parent` and test the result of a call to `get_parent` without having to take the addresses of the `node` objects:

```c++
node a, b;
b.set_parent(a)
assert(b.get_parent() == a);
```

Not only is the syntax nicer, but it discourages the use of raw pointers in client code, reducing the potential for `nullptr` bugs. Of course, we could have overloaded `set_parent` to take both `node*` _and_ `node&`, but who wants to do the extra work, or indeed, can remember to do it consistently for all APIs? And anyway, `get_parent` could not be overloaded to return `T&` as well (that would be both unsafe _and_ impossible by C++ overloading rules; and anyway, using `==` on a reference compares the referenced object, not the reference itself).

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

Instead of `node&` or `node*`, we can use `view<node>`, the non-optional counterpart to `optional_view<node>`. In addition to all the documentation benefits and compile-time assurances that `optional_view` provides, `view` _must_ always reference an object. An `optional_view<T>` _is_ a __const-correct__, __non-owning__, ___mandatory___ reference to a __single__ object of type `T`. And it can be stored in STL containers, just like `optional_view<T>` or `T*`.

Let's replace all instances of `node&` and `node*` with `view<node>`, and add the calls to `add_child` and `remove_child` to `set_parent`:

```c++
private:
    …
    std::vector<view<node>> children;
    
public:
    void set_parent(optional_view<node> new_parent) {
        if (parent) parent->remove_child(*this);
        parent = copy_view(new_parent);
        if (parent) parent->add_child(*this);
    }
    
    std::size_t get_child_count() const {
        return children.size();
    }

    view<node> get_child(std::size_t index) {
        return copy_view(children[index]);
    }

    view<node const> get_child(std::size_t index) const {
        return copy_view(children[index]);
    }

private:
    void add_child(view<node> child) {
        children.push_back(copy_view(child));
    }

    void remove_child(view<node> child) {
        children.erase(std::find(children.begin(), children.end(), child));
    }
};
```

As well as the implementation being safer, the way the API is used has become more consistent. Previously, different syntax would have been required to store and access `node&` and `node*`:

```c++
auto aa = b.get_parent();
auto& bb = a.get_child(0);

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

Of course, it is subjective as to whether or not this is an improvement; the requirement to use `operator->` instead of `operator.` where `view<T>` replaces `T&` could be considered a burden. Since `view<T>` and `operator_view<T>` implicitly convert to and from `T&` and `T*`, it is perfectly possible to restrict their use to your implementation, while using only `T&` and `T*` in your API.

### FAQ

#### Isn't `view` practically the same as `std::reference_wrapper`?

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
