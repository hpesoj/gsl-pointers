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

TEST_CASE("view_traits")
{
    // !!! Not working on MSVC
    //CHECK(std::is_trivially_copyable<view<int>>::value);
    //CHECK(std::is_trivially_copyable<optional_view<int>>::value);
}

TEST_CASE("view_construct")
{
    std::array<int, 2> i = { 0, 1 };

    view<int> v0 = i[0];
    view<int> v1 = i[1];

    CHECK(v0 == i[0]);
    CHECK(*v0 == i[0]);
    CHECK(*v0 == v0.value());
    CHECK(&*v0 == &i[0]);
    CHECK(&*v0 == &v0.value());

    CHECK(v1 == i[1]);
    CHECK(*v1 == i[1]);
    CHECK(*v1 == v1.value());
    CHECK(&*v1 == &i[1]);
    CHECK(&*v1 == &v1.value());

    CHECK(!(v0 == v1));
    CHECK(v0 != v1);
    CHECK(v0 < v1);
    CHECK(!(v0 > v1));
    CHECK(v0 <= v1);
    CHECK(!(v0 >= v1));

    CHECK(!(v1 == v0));
    CHECK(v1 != v0);
    CHECK(!(v1 < v0));
    CHECK(v1 > v0);
    CHECK(!(v1 <= v0));
    CHECK(v1 >= v0);

    CHECK(v0 == v0);
    CHECK(!(v0 != v0));
    CHECK(!(v0 < v0));
    CHECK(!(v0 > v0));
    CHECK(v0 <= v0);
    CHECK(v0 >= v0);
}

TEST_CASE("view_reassign")
{
    std::array<int, 2> i = { 0, 1 };

    view<int> v0 = i[0];
    view<int> v1 = i[0];

    CHECK(v0 == v1);
    CHECK(v0 == i[0]);
    CHECK(*v0 == i[0]);

    v0 = i[1];

    CHECK(v0 != v1);
    CHECK(v0 == i[1]);
    CHECK(*v0 == i[1]);

    v0 = v1;

    CHECK(v0 == v1);
    CHECK(v0 == i[0]);
    CHECK(*v0 == i[0]);
}

TEST_CASE("view_move")
{
    std::array<int, 2> i = { 0, 1 };

    view<int> v0 = i[0];
    view<int> v1 = std::move(v0);

    CHECK(v0 == v1);
    CHECK(v0 == i[0]);
    CHECK(*v0 == i[0]);

    v1 = i[1];
    v0 = std::move(v1);

    CHECK(v0 == v1);
    CHECK(v0 == i[1]);
    CHECK(*v0 == i[1]);
}

TEST_CASE("view_swap")
{
    using std::swap;

    std::array<int, 2> i = { 0, 1 };

    view<int> v0 = i[0];
    view<int> v1 = i[1];

    CHECK(v0 != v1);
    CHECK(v0 == i[0]);
    CHECK(v1 == i[1]);

    swap(v0, v1);

    CHECK(v0 != v1);
    CHECK(v0 == i[1]);
    CHECK(v1 == i[0]);

    swap(v0, v0);

    CHECK(v0 == i[1]);
}

TEST_CASE("view_static_cast")
{
    struct base { virtual ~base() {} };
    struct derived : base {};

    derived d;

    view<base> vb = d;
    view<derived> vd = static_view_cast<derived>(vb);

    CHECK(vb == d);
    CHECK(vd == d);
    CHECK(vd == vb);
}

TEST_CASE("view_const_cast")
{
    int i = 0;

    view<int const> vic = i;
    view<int> vi = const_view_cast<int>(vic);

    CHECK(vi == vic);
    CHECK(vi == i);
    CHECK(vic == i);
}

TEST_CASE("view_polymorphism")
{
    struct base { virtual ~base() {} };
    struct derived : base { explicit derived(int foo) : foo(foo) {} int foo; };

    base b0;
    derived d0{ 0 };
    derived d1{ 1 };

    view<base> vb = d0;
    view<derived> vd = d0;
    optional_view<derived> ovd;

    ovd = vd;

    CHECK(vb == vd);
    CHECK(vb == d0);

    vd = d1;

    CHECK(vb != vd);
    CHECK(vd == d1);
    CHECK(vd->foo == d1.foo);

    vb = vd;

    CHECK(vb == vd);
    CHECK(vb == d1);

    vd = d0;

    CHECK(vb != vd);
    CHECK(vd == d0);
    CHECK(vd->foo == d0.foo);

    vd = static_view_cast<derived>(vb);

    CHECK(vb == vd);
    CHECK(vd == d1);

    vd = d0;

    CHECK(vb != vd);
    CHECK(vd == d0);

    vd = dynamic_view_cast<derived>(vb);

    CHECK(vb == vd);
    CHECK(vd == d1);

    vb = b0;

    CHECK(vb != vd);
    CHECK(vb == b0);

    CHECK_THROWS(vd = dynamic_view_cast<derived>(vb));
    CHECK_THROWS(vd = dynamic_view_cast<derived>(vb));

    CHECK(vb == b0);
}

TEST_CASE("view_constexpr")
{
    //struct foo { int bar; };

    //constexpr static foo f = { 0 };

    //constexpr view<foo const> v0 = f;
    //constexpr view<foo const> v1 = v0;

    //constexpr int const& b = v0->bar;
    //constexpr foo const& f0 = *v0;
    //constexpr foo const& f1 = v0.value();
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
