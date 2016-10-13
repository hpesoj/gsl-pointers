# `view` and `optional_view`: reference types for C++

`view<T>` and `optional_view<T>` are C++ wrapper types intended to replace `T&` and `T*` wherever they are used as non-owning references that:

* Encourage writing compile-time safe code
* Allow for zero run-time overhead
* Perform run-time checks where required
* Provide an intuitive and cohesive interface

## Quick example

```c++
void 
```

## What's wrong with `T&` and `T*`?



## FAQ

### Isn't `view` practically the same as `std::reference_wrapper`?

Both types perform a similar function, but there are differences.

Both `view<T>` and `std::reference_wrapper<T>` can be constructed from `T&`:

```c++
foo f;

view<foo> v = f;
reference_wrapper<foo> rw = f;
```

And both `view<T>` and `std::reference_wrapper<T>` implicitly convert to `T&`:

```c++
foo& a = v;
foo& b = rw;
```

But `view<T>` also implicitly converts to `T*`:

```c++
foo* a = v;
foo* b = rw; // error
```

And conversion from `view<T>` to `T` is prohibited:

```c++
foo a = v; // error
foo b = rw;
```

With `view`, it's necessary to use `operator*` or `operator->` to access the underlying value directly. The equivalent with `std::reference_wrapper` is the `get` function.

```c++
v->bar = 42;
rw.get().bar = 42;

foo a = *v;
foo b = rw.get();
```

In addition, `view<T>` defines `operator bool`, which always returns `true`, and `get`, which returns a pointer to the wrapped object. This allows `view<T>` to be used with the proposed `std::propagate_const` wrapper, and with other generic code that expects a (smart) pointer-like interface.
