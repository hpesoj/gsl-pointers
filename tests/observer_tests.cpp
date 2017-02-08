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

#include <observer.hpp>
#include <optional_ref.hpp>

#include <algorithm>
#include <array>
#include <cctype>
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

    derived(derived const&) = default;
    derived(derived&& other) :
        foo(other.foo)
    {
        other.foo = 0;
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

using gsl::nullopt;
using gsl::optional_ref;
using gsl::make_optional_ref;

SCENARIO("`optional_ref` can be constructed")
{
    int i = {};
    int j = {};

    GIVEN("an default constructed `optional_ref`")
    {
        optional_ref<int> o;

        CHECK(!o);

        CHECK(o == nullopt);
        CHECK(nullopt == o);
        CHECK(!(o == i));
        CHECK(!(o == j));

        CHECK(!(o != nullopt));
        CHECK(!(nullopt != o));
        CHECK(o != i);
        CHECK(o != j);
    }

    GIVEN("an `optional_ref` constructed from a reference")
    {
        optional_ref<int> o = i;

        CHECK(o);

        CHECK(!(o == nullopt));
        CHECK(!(nullopt == o));
        CHECK(o == i);
        CHECK(o == j);
        CHECK(*o == i);
        CHECK(*o == j);

        CHECK(o != nullopt);
        CHECK(nullopt != o);
        CHECK(!(o != i));
        CHECK(!(o != j));
        CHECK(!(*o != i));
        CHECK(!(*o != j));

        CHECK(&*o == &i);
        CHECK(&*o != &j);
    }
}

using gsl::observer;
using gsl::make_observer;

SCENARIO("`observer` are trivially copyable")
{
    CHECK(std::is_trivially_copyable<observer<int>>::value);
}

SCENARIO("observers can be constructed")
{
    int i = {};
    int j = {};

    GIVEN("a observer implicitly constructed from a reference")
    {
        observer<int> v = make_observer(i);

        REQUIRE(v == make_observer(i));
        REQUIRE(v != make_observer(j));

        WHEN("is is assigned a reference")
        {
            v = make_observer(j);

            REQUIRE(v == make_observer(j));
            REQUIRE(v != make_observer(i));
        }
    }
}

SCENARIO("observers convert to pointers")
{
    int i = {};

    GIVEN("a observer constructed from an reference")
    {
        observer<int> v = make_observer(i);

        WHEN("the observer is converted to a pointer")
        {
            int* p = static_cast<int*>(v);

            REQUIRE(p == &i);
        }
    }
}

SCENARIO("observers can be copied")
{
    int i = {};
    int j = {};

    GIVEN("a copy constructed observer")
    {
        observer<int> v = make_observer(i);
        observer<int> w = v;

        REQUIRE(w == v);

        REQUIRE(w == make_observer(i));
        REQUIRE(w != make_observer(j));

        REQUIRE(v == make_observer(i));
        REQUIRE(v != make_observer(j));

        WHEN("it is copy assigned")
        {
            observer<int> x = make_observer(j);
            w = x;

            REQUIRE(w == x);

            REQUIRE(w == make_observer(j));
            REQUIRE(w != make_observer(i));

            REQUIRE(x == make_observer(j));
            REQUIRE(x != make_observer(i));

            REQUIRE(v == make_observer(i));
            REQUIRE(v != make_observer(j));
        }
    }
}

SCENARIO("observers can be moved")
{
    int i = {};
    int j = {};

    GIVEN("a move constructed observer")
    {
        observer<int> v = make_observer(i);
        observer<int> w = std::move(v);

        REQUIRE(w == v);

        REQUIRE(w == make_observer(i));
        REQUIRE(w != make_observer(j));

        REQUIRE(v == make_observer(i));
        REQUIRE(v != make_observer(j));

        WHEN("it is move assigned")
        {
            observer<int> x = make_observer(j);
            w = std::move(x);

            REQUIRE(w == x);

            REQUIRE(w == make_observer(j));
            REQUIRE(w != make_observer(i));

            REQUIRE(x == make_observer(j));
            REQUIRE(x != make_observer(i));

            REQUIRE(v == make_observer(i));
            REQUIRE(v != make_observer(j));
        }
    }
}

SCENARIO("observers can be swapped")
{
    int i = {};
    int j = {};

    GIVEN("a observer swapped with a observer")
    {
        observer<int> v = make_observer(i);
        observer<int> w = make_observer(j);

        swap(v, w);

        REQUIRE(v == make_observer(j));
        REQUIRE(w == make_observer(i));
    }

    GIVEN("a observer swapped with itself")
    {
        observer<int> v = make_observer(i);

        swap(v, v);

        REQUIRE(v == make_observer(i));
    }
}

SCENARIO("observers can be used to access the objects they reference")
{
    int i = 1;
    int j = 2;

    GIVEN("a observer constructed from an reference")
    {
        observer<int> v = make_observer(i);

        REQUIRE(v == make_observer(i));
        REQUIRE(v != make_observer(j));
        REQUIRE(*v == 1);
        REQUIRE(*v == i);
        REQUIRE(*v != j);

        WHEN("it is assigned an reference")
        {
            v = make_observer(j);

            REQUIRE(v == make_observer(j));
            REQUIRE(v != make_observer(i));
            REQUIRE(*v == 2);
            REQUIRE(*v == j);
            REQUIRE(*v != i);

            WHEN("the observered object is assigned a reference")
            {
                *v = i;

                REQUIRE(v == make_observer(j));
                REQUIRE(v != make_observer(i));
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

SCENARIO("observers support equality and `less` comparison")
{
    int i = {};
    std::array<int, 2> is = { 1, 2 };

    GIVEN("an observer constructed from an entry in an array")
    {
        THEN("`operator==` is supported")
        {
            REQUIRE(make_observer(is[0]) == make_observer(is[0]));
            REQUIRE(!(make_observer(is[0]) == make_observer(is[1])));
            REQUIRE(!(make_observer(is[1]) == make_observer(is[0])));
        }

        THEN("`operator!=` is supported")
        {
            REQUIRE(!(make_observer(is[0]) != make_observer(is[0])));
            REQUIRE(make_observer(is[0]) != make_observer(is[1]));
            REQUIRE(make_observer(is[1]) != make_observer(is[0]));
        }

        THEN("`less` is supported")
        {
            REQUIRE(!(std::less<observer<int>>()(make_observer(is[0]), make_observer(is[0]))));
            REQUIRE(std::less<observer<int>>()(make_observer(is[0]), make_observer(is[1])));
            REQUIRE(!(std::less<observer<int>>()(make_observer(is[1]), make_observer(is[0]))));
        }
    }
}

SCENARIO("observers can be created using `make_observer`")
{
    int i = {};

    GIVEN("a observer created with `make_observer`")
    {
        observer<int> v = make_observer(i);

        REQUIRE(v == make_observer(i));
    }
}

SCENARIO("observers can be used in certain constant expressions")
{
    // !!! Not working on MSVC
    //constexpr static int const i = {};
    //constexpr observer<int const> const o = make_observer(i);
    //constexpr int j = *o;
}

SCENARIO("observers can be used with STL containers")
{
    std::array<int, 3> i = { 0, 1, 2 };

    GIVEN("a `vector` of observers")
    {
        std::vector<observer<int>> vector;

        vector.emplace_back(i[2]);
        vector.emplace_back(i[1]);
        vector.emplace_back(i[0]);

        REQUIRE(vector[0] == make_observer(i[2]));
        REQUIRE(vector[1] == make_observer(i[1]));
        REQUIRE(vector[2] == make_observer(i[0]));
    }

    GIVEN("a `map` of observer-observer pairs")
    {
        std::map<observer<int>, observer<int>> map;

        map.emplace(i[0], i[2]);
        map.emplace(i[1], i[1]);
        map.emplace(i[2], i[0]);

        REQUIRE(map.at(make_observer(i[0])) == make_observer(i[2]));
        REQUIRE(map.at(make_observer(i[1])) == make_observer(i[1]));
        REQUIRE(map.at(make_observer(i[2])) == make_observer(i[0]));
    }

    GIVEN("an `unordered_map` of observer-observer pairs")
    {
        std::unordered_map<observer<int>, observer<int>> map;

        map.emplace(i[0], i[2]);
        map.emplace(i[1], i[1]);
        map.emplace(i[2], i[0]);

        REQUIRE(map.at(make_observer(i[0])) == make_observer(i[2]));
        REQUIRE(map.at(make_observer(i[1])) == make_observer(i[1]));
        REQUIRE(map.at(make_observer(i[2])) == make_observer(i[0]));
    }

    GIVEN("a `set` of observers")
    {
        std::set<observer<int>> set;

        set.emplace(i[0]);
        set.emplace(i[1]);
        set.emplace(i[2]);

        REQUIRE(set.find(make_observer(i[0])) != set.end());
        REQUIRE(set.find(make_observer(i[1])) != set.end());
        REQUIRE(set.find(make_observer(i[2])) != set.end());
    }

    GIVEN("an `unordered_set` of observers")
    {
        std::unordered_set<observer<int>> set;

        set.emplace(i[0]);
        set.emplace(i[1]);
        set.emplace(i[2]);

        REQUIRE(set.find(make_observer(i[0])) != set.end());
        REQUIRE(set.find(make_observer(i[1])) != set.end());
        REQUIRE(set.find(make_observer(i[2])) != set.end());
    }
}
