/*
 * Copyright (c) 2016 Joseph Thomson
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"

#include <view.hpp>

#include <algorithm>
#include <array>
#include <iostream>
#include <map>
#include <unordered_map>
#include <vector>

namespace
{

struct base
{
    virtual ~base() {}
};

struct derived : base
{
    int foo = {};

    derived() = default;

    explicit derived(int foo) :
        foo(foo)
    {
    }
};

struct derived_other : base
{
    int foo = {};

    derived_other() = default;

    explicit derived_other(int foo) :
        foo(foo)
    {
    }
};

} // namespace

SCENARIO("`view` and `optional_view` are trivially copyable")
{
    // !!! Not working on MSVC
    //CHECK(std::is_trivially_copyable<view<int>>::value);
    //CHECK(std::is_trivially_copyable<optional_view<int>>::value);
}

SCENARIO("`view` can reference objects")
{
    int i = {};
    int j = {};

    GIVEN("a `view<int>` constructed from an `int&`")
    {
        view<int> v = i;

        REQUIRE(v == i);
        REQUIRE(v != j);

        WHEN("is is assigned an `int&`")
        {
            v = j;

            REQUIRE(v == j);
            REQUIRE(v != i);
        }
    }
}

SCENARIO("`view` can be copied")
{
    int i = {};
    int j = {};

    GIVEN("a `view<int>` copy constructed from a `view<int>`")
    {
        view<int> v = i;
        view<int> w = v;

        REQUIRE(w == v);

        REQUIRE(w == i);
        REQUIRE(w != j);

        REQUIRE(v == i);
        REQUIRE(v != j);

        WHEN("it is copy assigned a `view<int>`")
        {
            view<int> x = j;
            w = x;

            REQUIRE(w == x);

            REQUIRE(w == j);
            REQUIRE(w != i);

            REQUIRE(x == j);
            REQUIRE(x != i);

            REQUIRE(v == i);
            REQUIRE(v != j);
        }

        WHEN("it is copy assigned itself")
        {
            w = w;

            REQUIRE(w == v);

            REQUIRE(w == i);
            REQUIRE(w != j);

            REQUIRE(v == i);
            REQUIRE(v != j);
        }
    }
}

SCENARIO("`view` can be moved")
{
    int i = {};
    int j = {};

    GIVEN("a `view<int>` move constructed from a `view<int>`")
    {
        view<int> v = i;
        view<int> w = std::move(v);

        REQUIRE(w == v);

        REQUIRE(w == i);
        REQUIRE(w != j);

        REQUIRE(v == i);
        REQUIRE(v != j);

        WHEN("it is move assigned a `view<int>`")
        {
            view<int> x = j;
            w = std::move(x);

            REQUIRE(w == x);

            REQUIRE(w == j);
            REQUIRE(w != i);

            REQUIRE(x == j);
            REQUIRE(x != i);

            REQUIRE(v == i);
            REQUIRE(v != j);
        }

        WHEN("it is move assigned itself")
        {
            w = std::move(w);

            REQUIRE(w == v);

            REQUIRE(w == i);
            REQUIRE(w != j);

            REQUIRE(v == i);
            REQUIRE(v != j);
        }
    }
}

SCENARIO("`view` can be swapped")
{
    int i = {};
    int j = {};

    GIVEN("a `view<int>` swapped with a `view<int>`")
    {
        view<int> v = i;
        view<int> w = j;

        swap(v, w);

        REQUIRE(v == j);
        REQUIRE(w == i);
    }

    GIVEN("a `view<int>` swapped with itself")
    {
        view<int> v = i;

        swap(v, v);

        REQUIRE(v == i);
    }
}

SCENARIO("`view` can be used to access the objects it references")
{
    int i = 1;
    int j = 2;

    GIVEN("a `view<int>` constructed from an `int&`")
    {
        view<int> v = i;

        REQUIRE(v == i);
        REQUIRE(v != j);
        REQUIRE(*v == 1);
        REQUIRE(*v == i);
        REQUIRE(*v != j);

        WHEN("is is assigned an `int&`")
        {
            v = j;

            REQUIRE(v == j);
            REQUIRE(v != i);
            REQUIRE(*v == 2);
            REQUIRE(*v == j);
            REQUIRE(*v != i);

            WHEN("is the referenced object is assigned an `int`")
            {
                *v = i;

                REQUIRE(v == j);
                REQUIRE(v != i);
                REQUIRE(*v == 1);
                REQUIRE(*v == i);
                REQUIRE(*v == j);
                REQUIRE(i == 1);
                REQUIRE(j == 1);
                REQUIRE(i == j);
            }
        }
    }
}

SCENARIO("`view` converts to `T&` and `T*`")
{
    int i = 1;

    GIVEN("a `view<int>` constructed from an `int&`")
    {
        view<int> v = i;

        THEN("the `view<int>` is implicitly converted to `int&`")
        {
            int& r = v;

            REQUIRE(&r == &i);
        }

        THEN("the `view<int>` is implicitly converted to `int const&`")
        {
            int const& r = v;

            REQUIRE(&r == &i);
        }

        THEN("the `view<int>` is explicitly converted to `int*`")
        {
            int* p = static_cast<int*>(v);

            REQUIRE(p == &i);
        }

        THEN("the `view<int>` is explicitly converted to `int const*`")
        {
            int const* p = static_cast<int const*>(v);

            REQUIRE(p == &i);
        }
    }
}

SCENARIO("`view` supports arithmetic comparison")
{
    std::array<int, 2> is = { 1, 2 };

    GIVEN("two `view<int>`s constructed from entries in an array")
    {
        view<int> u = is[0];
        view<int> v = is[0];
        view<int> w = is[1];

        THEN("`operator==` is supported")
        {
            REQUIRE(v == v);
            REQUIRE(u == v);
            REQUIRE(v == u);
            REQUIRE(!(v == w));
            REQUIRE(!(w == v));
        }

        THEN("`operator!=` is supported")
        {
            REQUIRE(!(v != v));
            REQUIRE(!(u != v));
            REQUIRE(!(v != u));
            REQUIRE(v != w);
            REQUIRE(w != v);
        }

        THEN("`operator<` is supported")
        {
            REQUIRE(!(v < v));
            REQUIRE(!(u < v));
            REQUIRE(!(v < u));
            REQUIRE(v < w);
            REQUIRE(!(w < v));
        }

        THEN("`operator<=` is supported")
        {
            REQUIRE(v <= v);
            REQUIRE(u <= v);
            REQUIRE(v <= u);
            REQUIRE(v <= w);
            REQUIRE(!(w <= v));
        }

        THEN("`operator>` is supported")
        {
            REQUIRE(!(v > v));
            REQUIRE(!(u > v));
            REQUIRE(!(v > u));
            REQUIRE(!(v > w));
            REQUIRE(w > v);
        }

        THEN("`operator>=` is supported")
        {
            REQUIRE(v >= v);
            REQUIRE(u >= v);
            REQUIRE(v >= u);
            REQUIRE(!(v >= w));
            REQUIRE(w >= v);
        }
    }
}

SCENARIO("`view` can be static cast")
{
    derived d;

    GIVEN("a `view<base>` initialized with `derived&`")
    {
        view<base> v = d;

        WHEN("it is static cast to a `view<derived>`")
        {
            view<derived> w = static_view_cast<derived>(v);

            REQUIRE(w == v);
            REQUIRE(w == d);
        }
    }
}

SCENARIO("`view` can be dynamic cast")
{
    derived d;

    GIVEN("a `view<base>` initialized with `derived&`")
    {
        view<base> v = d;

        WHEN("it is dynamic cast to a `view<derived>`")
        {
            view<derived> w = static_view_cast<derived>(v);

            REQUIRE(w == v);
            REQUIRE(w == d);
        }

        WHEN("it is dynamic cast to a `view<derived_other>`")
        {
            REQUIRE_THROWS(dynamic_view_cast<derived_other>(v));

            REQUIRE(v == d);
        }
    }
}

SCENARIO("`view` can be const cast")
{
    int i = {};

    GIVEN("a `view<int const>` initialized with `int&`")
    {
        view<int> v = i;

        WHEN("it is const cast to a `view<int>`")
        {
            view<int> w = const_view_cast<int>(v);

            REQUIRE(w == v);
        }
    }
}

SCENARIO("`view` can be used in certain constant expressions")
{
    // !!! Not working on MSVC
    //constexpr static derived d = { 0 };

    //constexpr view<foo const> v = d;
    //constexpr view<foo const> w = v;

    //constexpr int const& b = v->foo;
    //constexpr derived const& r1 = *v;
    //constexpr derived const& r2 = v.value();
}

TEST_CASE("view_unordered_map")
{
    std::array<int, 3> i = { 0, 1, 2 };
    std::unordered_map<view<int>, int> map;

    map[i[0]] = 1;
    map[i[1]] = 2;
    map[i[2]] = 4;

    CHECK(map[i[0]] == 1);
    CHECK(map[i[1]] == 2);
    CHECK(map[i[2]] == 4);
}

TEST_CASE("view_set")
{
    std::array<int, 3> i = { 0, 1, 2 };
    std::map<std::string, optional_view<int>> m;

    m["a"] = i[0];
    m["b"] = i[1];
    m["c"] = i[2];

    int* b = get_pointer(m["b"]);

    CHECK(b == &i[1]);
}

TEST_CASE("view_map")
{
    std::array<int, 3> i = { 0, 1, 2 };
    std::map<view<int>, int> map;

    map[i[0]] = 1;
    map[i[1]] = 2;
    map[i[2]] = 4;

    CHECK(map[i[0]] == 1);
    CHECK(map[i[1]] == 2);
    CHECK(map[i[2]] == 4);
}

TEST_CASE("view_vector")
{
    std::array<int, 3> i = { 0, 1, 2 };
    std::vector<view<int>> vec;

    vec.push_back(i[0]);
    vec.push_back(i[1]);
    vec.push_back(i[2]);

    CHECK(vec[0] == i[0]);
    CHECK(vec[1] == i[1]);
    CHECK(vec[2] == i[2]);
}

TEST_CASE("implicit conversion")
{
    auto fr = [](int& i)
    {
        i += 1;
    };

    int i = 0;
    view<int> v = i;

    fr(v);
    CHECK(i == 1);
}

TEST_CASE("range-based for loop")
{
    std::array<int, 3> i = { 0, 1, 2 };
    std::vector<view<int>> vec;
    vec.emplace_back(i[0]);
    vec.emplace_back(i[1]);
    vec.emplace_back(i[2]);

    for (auto i : vec) {
        *i *= *i;
    }

    CHECK(i[0] == 0);
    CHECK(i[1] == 1);
    CHECK(i[2] == 4);

    for (int& i : vec) {
        i += 1;
    }

    CHECK(i[0] == 1);
    CHECK(i[1] == 2);
    CHECK(i[2] == 5);
}

void peanuts(int const& v)
{
    std::cout << "peanuts = " << v << std::endl;
}

void fresh(int const* v)
{
    if (v)
        std::cout << "fresh = " << *v << std::endl;
    else
        std::cout << "fresh = {}" << std::endl;
}

void funky(optional_view<int const> v)
{
    if (v)
        std::cout << "funky = " << *v << std::endl;
    else
        std::cout << "funky = {}" << std::endl;
}

void jam(view<int const> v)
{
    std::cout << "jam = " << *v << std::endl;
}

TEST_CASE("make functions")
{
    int i = 0;
    int j = 0;

    view<int> v0 = i;
    view<int> v1 = make_view(i);

    CHECK(v0 == v1);

    CHECK(v0 == i);
    CHECK(v0 != j);

    v0 = j;

    CHECK(v0 != i);
    CHECK(v0 == j);

    optional_view<int> ov0 = i;
    optional_view<int> ov1 = make_optional_view(i);

    struct base { virtual ~base() {} };
    struct derived : base { explicit derived(int foo) : foo(foo) {} int foo; };

    derived d{ 1 };

    optional_view<int> test;

    int const& f = test.value_or(2);

    struct fab {
        view<int const> toost;
    };

    fab frab = { i };

    int iii = frab.toost;

    test = {};

    funky({});
    funky(i);
    funky(42);

    fresh({});
    fresh(&i);
    //fresh(&42);

    //jam({});
    jam(i);
    jam(42);

    peanuts({});
    peanuts(i);
    peanuts(42);

    CHECK(ov0 == ov1);
    //CHECK(v0 == ov0);
}

int gint = 1;

void take_pointer(int* i) {}
int& return_reference() { return gint; }

template <typename InputIt>
auto sorted_view(InputIt first, InputIt last) {
    using value_type = typename std::iterator_traits<InputIt>::value_type;
    std::vector<view<value_type const>> v(first, last);
    std::sort(std::begin(v), std::end(v), std::less<value_type const&>());
    return v;
}

TEST_CASE("proxy sort")
{
    int const values[] = { 4, 8, 1, 5, 2, 9, 3 };
    auto views = sorted_view(std::begin(values), std::end(values));

    for (int const& v : views) {
        std::cout << v << "\n";
    }

    struct foo { int bar; };

    foo f;

    std::reference_wrapper<foo> r = f;
    r.get().bar = 42;

    view<foo> v = f;
    v->bar = 42;

    take_pointer(&return_reference());

    view<int const> noob = values[0];

    noob == values[0];
}

class node {
private:
    optional_view<node> parent;
    std::vector<view<node>> children;

public:
    void set_parent(optional_view<node> new_parent) {
        if (parent) parent->remove_child(*this);
        parent = new_parent;
        if (parent) parent->add_child(*this);
    }

    optional_view<node> get_parent() const {
        return const_view_cast<node>(parent);
    }

    std::size_t child_count() const {
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
        children.emplace_back(child);
    }

    void remove_child(view<node> child) {
        children.erase(std::find(children.begin(), children.end(), child));
    }
};

TEST_CASE("node tree")
{
    node a, b, c;

    a.set_parent(b);
    b.set_parent(c);

    optional_view<node> b0 = a.get_parent();

    view<node> a0 = b.get_child(0);
    view<node const> a1 = *a0;

    node const* x = get_pointer(a0);
    auto& y = b0.value();

    if (b.get_parent() == c) {
        for (auto i = 0u; i < b.child_count(); ++i) {
            auto child = b.get_child(i);
        }
    }
}

class node_old {
private:
    std::vector<node_old*> children;
    node_old* parent = nullptr;

public:
    node_old() = default;
    node_old(node_old const&) = delete;
    node_old& operator=(node_old const&) = delete;

    void set_parent(node_old* node) {
        if (parent) parent->remove_child(*this);
        parent = node;
        if (parent) parent->add_child(*this);
    }

    node_old* get_parent() {
        return parent;
    }

    node_old const* get_parent() const {
        return parent;
    }

    std::size_t child_count() const {
        return children.size();
    }

    node_old& get_child(std::size_t index) {
        return *children[index];
    }

    node_old const& get_child(std::size_t index) const {
        return *children[index];
    }

private:
    void add_child(node_old& node) {
        children.push_back(&node);
    }

    void remove_child(node_old& node) {
        children.erase(std::find(children.begin(), children.end(), &node));
    }
};

TEST_CASE("node tree")
{
    node_old a, b, c;

    a.set_parent(&b);
    b.set_parent(&c);

    auto& d = b.get_child(0u);

    if (b.get_parent() == &c) {
        while (b.child_count() > 0u) {
            auto& child = b.get_child(0);
            child.set_parent(&c);
        }
    }
}
