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
#include <set>
#include <unordered_map>
#include <unordered_set>
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

SCENARIO("`view` constructs from `T&` and `T*`")
{
    int i = {};
    int j = {};
    int* p = &i;
    int* n = {};

    GIVEN("a `view<int>` implicitly constructed from an `int&`")
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

    GIVEN("a `view<int>` explicitly constructed from an `int*`")
    {
        view<int> v{p};

        REQUIRE(v == i);
        REQUIRE(v != j);
    }

    GIVEN("a `view<int>` explicitly constructed from a null `int*`")
    {
        REQUIRE_THROWS(view<int> v{n});
    }

    GIVEN("a `view<int>` explicitly constructed from a `nullptr`")
    {
        REQUIRE_THROWS(view<int> v{nullptr});
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
            REQUIRE(v == is[0]);
            REQUIRE(!(v == is[1]));
            REQUIRE(v == v);
            REQUIRE(u == v);
            REQUIRE(v == u);
            REQUIRE(!(v == w));
            REQUIRE(!(w == v));
        }

        THEN("`operator!=` is supported")
        {
            REQUIRE(!(v != is[0]));
            REQUIRE(v != is[1]);
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

SCENARIO("`view` can be created using `make_view`")
{
    int i = 42;
    int const c = 21;

    GIVEN("a `view<int>` created with `make_view` from an `int`")
    {
        auto v = make_view(i);

        REQUIRE((std::is_same<decltype(v), view<int>>::value));
        REQUIRE(v == i);
        REQUIRE(*v == 42);
    }

    GIVEN("a `view<int>` created with `make_view` from an `int const`")
    {
        auto v = make_view(c);

        REQUIRE((std::is_same<decltype(v), view<int const>>::value));
        REQUIRE(v == c);
        REQUIRE(*v == 21);
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

SCENARIO("`view` can be used with STL containers")
{
    std::array<int, 3> i = { 0, 1, 2 };

    GIVEN("a `vector<view<int>>`")
    {
        std::vector<view<int>> vector;

        vector.emplace_back(i[2]);
        vector.emplace_back(i[1]);
        vector.emplace_back(i[0]);

        REQUIRE(vector[0] == i[2]);
        REQUIRE(vector[1] == i[1]);
        REQUIRE(vector[2] == i[0]);
    }

    GIVEN("a `map<view<int>, view<int>>`")
    {
        std::map<view<int>, view<int>> map;

        map.emplace(i[0], i[2]);
        map.emplace(i[1], i[1]);
        map.emplace(i[2], i[0]);

        REQUIRE(map.at(i[0]) == i[2]);
        REQUIRE(map.at(i[1]) == i[1]);
        REQUIRE(map.at(i[2]) == i[0]);
    }

    GIVEN("an `unordered_map<view<int>, view<int>>`")
    {
        std::unordered_map<view<int>, view<int>> map;

        map.emplace(i[0], i[2]);
        map.emplace(i[1], i[1]);
        map.emplace(i[2], i[0]);

        REQUIRE(map.at(i[0]) == i[2]);
        REQUIRE(map.at(i[1]) == i[1]);
        REQUIRE(map.at(i[2]) == i[0]);
    }

    GIVEN("a `set<view<int>>`")
    {
        std::set<view<int>> set;

        set.emplace(i[0]);
        set.emplace(i[1]);
        set.emplace(i[2]);

        REQUIRE(set.find(i[0]) != set.end());
        REQUIRE(set.find(i[1]) != set.end());
        REQUIRE(set.find(i[2]) != set.end());
    }

    GIVEN("an `unordered_set<view<int>>`")
    {
        std::unordered_set<view<int>> set;

        set.emplace(i[0]);
        set.emplace(i[1]);
        set.emplace(i[2]);

        REQUIRE(set.find(i[0]) != set.end());
        REQUIRE(set.find(i[1]) != set.end());
        REQUIRE(set.find(i[2]) != set.end());
    }
}

SCENARIO("containers of `view`s can be used with range-based for loops")
{
    std::array<int, 3> i = { 0, 1, 2 };

    GIVEN("a `vector<view<int>>`")
    {
        std::vector<view<int>> vector;

        vector.emplace_back(i[0]);
        vector.emplace_back(i[1]);
        vector.emplace_back(i[2]);

        for (auto i : vector) {
            *i += 1;
        }

        CHECK(i[0] == 1);
        CHECK(i[1] == 2);
        CHECK(i[2] == 3);

        for (int& i : vector) {
            i += 1;
        }

        CHECK(i[0] == 2);
        CHECK(i[1] == 3);
        CHECK(i[2] == 4);
    }
}

template <typename InputIt>
auto sorted_view(InputIt first, InputIt last) {
    using value_type = typename std::iterator_traits<InputIt>::value_type;
    std::vector<view<value_type const>> v(first, last);
    std::sort(std::begin(v), std::end(v), std::less<value_type const&>());
    return v;
}

SCENARIO("`view` can be used to provide sorted views of arrays")
{
    std::array<int, 5> i = { 4, 8, 1, 5, 2 };

    auto views = sorted_view(std::begin(i), std::end(i));

    REQUIRE(views[0] == i[2]);
    REQUIRE(views[1] == i[4]);
    REQUIRE(views[2] == i[0]);
    REQUIRE(views[3] == i[3]);
    REQUIRE(views[4] == i[1]);
}

class node
{
private:
    optional_view<node> parent;
    std::vector<view<node>> children;

public:
    node() = default;

    node(node const&) = delete;
    node& operator=(node const&) = delete;

    node(node&&) = delete;
    node& operator=(node&&) = delete;

    void set_parent(optional_view<node> new_parent) {
        if (parent) parent->remove_child(*this);
        parent = new_parent;
        if (parent) parent->add_child(*this);
    }

    optional_view<node> get_parent() const {
        return parent;
    }

    std::size_t get_child_count() const {
        return children.size();
    }

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

SCENARIO("`view` and `optional_view` can be used to create a `node` type")
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
        for (auto i = 0u; i < b.get_child_count(); ++i) {
            auto child = b.get_child(i);
        }
    }
}
