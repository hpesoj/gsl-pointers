# `view` and `optional_view`: reference types for C++

`view<T>` and `optional_view<T>` are wrappers intended to replace `T&` and `T*` wherever they are used as non-owning references.

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
