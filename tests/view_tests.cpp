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

#include <jrt/view.hpp>

#include <array>
#include <iostream>
#include <map>
#include <unordered_map>
#include <vector>

using namespace jrt;

TEST_CASE("view_traits")
{
    CHECK(std::is_trivially_copyable<view<int>>::value);
    CHECK(std::is_trivially_copyable<optional_view<int>>::value);
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

TEST_CASE("view_const_cast")
{
    using std::swap;

    int i = 0;

    view<int const> vic = i;
    view<int> vi = const_view_cast<int>(vic);

    CHECK(vi == vic);
    CHECK(vi == i);
    CHECK(vic == i);
}

TEST_CASE("view_reinterpret_cast")
{
    using std::swap;

    int i = 0;

    view<int> vi = i;
    view<char> vc = reinterpret_view_cast<char>(vi);

    CHECK(vc == reinterpret_cast<char&>(i));
}

TEST_CASE("view_polymorphism")
{
    struct base { virtual ~base() {} };
    struct derived : base { explicit derived(int foo) : foo(foo) {} int foo; };

    base b0;
    derived d0{0};
    derived d1{1};

    view<base> vb = d0;
    view<derived> vd = d0;
    optional_view<derived> ovd;

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

    ovd = dynamic_view_cast<derived>(vb);

    CHECK(ovd);
    CHECK(vb == ovd);
    CHECK(ovd == d1);

    vb = b0;

    CHECK(vb != vd);
    CHECK(vb == b0);

    ovd = dynamic_view_cast<derived>(vb);

    CHECK(!ovd);
    CHECK(vb != ovd);
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

    int* b = m["b"];

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

    auto fp = [](int* i)
    {
        *i += 2;
    };

    int i = 0;
    view<int> v = i;
    optional_view<int> ov = i;

    fr(v);
    CHECK(i == 1);
    fp(v);
    CHECK(i == 3);

    fp(ov);
    CHECK(i == 5);
}

TEST_CASE("range-based for loop")
{
    std::array<int, 3> i = { 0, 1, 2 };
    std::vector<view<int>> vec = { i[0], i[1], i[2] };

    for (int& i : vec) {
        i *= i;
    }

    CHECK(*vec[0] == 0);
    CHECK(*vec[1] == 1);
    CHECK(*vec[2] == 4);

    for (int* i : vec) {
        *i += 1;
    }

    CHECK(*vec[0] == 1);
    CHECK(*vec[1] == 2);
    CHECK(*vec[2] == 5);

//    view<int> asd = std::move(i[0]);
}

TEST_CASE("make functions")
{
    int i = 0;

    view<int> v0 = i;
    view<int> v1 = make_view(i);

    CHECK(v0 == v1);

    optional_view<int> ov0 = i;
    optional_view<int> ov1 = make_optional_view(i);
    optional_view<int> ov2 = make_optional_view(&i);

    CHECK(ov0 == ov1);
    CHECK(ov1 == ov2);
    CHECK(v0 == ov0);
}
