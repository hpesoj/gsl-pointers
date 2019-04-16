/*
 * Copyright (c) 2016 - 2019 Joseph Thomson
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include <catch2/catch.hpp>

#include <gsl/retained.hpp>

#include <algorithm>
#include <array>
#include <map>
#include <set>
#include <unordered_map>
#include <unordered_set>
#include <vector>

using gsl::make_retained;
using gsl::retained;

SCENARIO("`retained` is trivially movable")
{
  CHECK(std::is_trivially_move_constructible_v<retained<int>>);
  CHECK(std::is_trivially_move_assignable_v<retained<int>>);
}

SCENARIO("`retained`s can be constructed")
{
  int i = {};
  int j = {};

  GIVEN("an `retained` implicitly constructed from a reference")
  {
    retained<int> v = make_retained(i);

    REQUIRE(v == make_retained(i));
    REQUIRE(v != make_retained(j));

    WHEN("is is assigned a reference")
    {
      v = make_retained(j);

      REQUIRE(v == make_retained(j));
      REQUIRE(v != make_retained(i));
    }
  }
}

SCENARIO("`retained`s cannot be copied")
{
  CHECK_FALSE(std::is_copy_constructible_v<retained<int>>);
  CHECK_FALSE(std::is_copy_assignable_v<retained<int>>);
}

SCENARIO("`retained`s can be moved")
{
  int i = {};
  int j = {};

  GIVEN("a move constructed `retained`")
  {
    retained<int> v = make_retained(i);
    retained<int> w = std::move(v);

    REQUIRE(w == v);

    REQUIRE(w == make_retained(i));
    REQUIRE(w != make_retained(j));

    REQUIRE(v == make_retained(i));
    REQUIRE(v != make_retained(j));

    WHEN("it is move assigned")
    {
      retained<int> x = make_retained(j);
      w = std::move(x);

      REQUIRE(w == x);

      REQUIRE(w == make_retained(j));
      REQUIRE(w != make_retained(i));

      REQUIRE(x == make_retained(j));
      REQUIRE(x != make_retained(i));

      REQUIRE(v == make_retained(i));
      REQUIRE(v != make_retained(j));
    }
  }
}

SCENARIO("`retained`s can be swapped")
{
  int i = {};
  int j = {};

  GIVEN("an `retained` swapped with an `retained`")
  {
    retained<int> v = make_retained(i);
    retained<int> w = make_retained(j);

    swap(v, w);

    REQUIRE(v == make_retained(j));
    REQUIRE(w == make_retained(i));
  }

  GIVEN("an `retained` swapped with itself")
  {
    retained<int> v = make_retained(i);

    swap(v, v);

    REQUIRE(v == make_retained(i));
  }
}

SCENARIO("`retained`s can be used to access the objects they reference")
{
  int i = 1;
  int j = 2;

  GIVEN("an `retained` constructed from an reference")
  {
    retained<int> v = make_retained(i);

    REQUIRE(v == make_retained(i));
    REQUIRE(v != make_retained(j));
    REQUIRE(*v == 1);
    REQUIRE(*v == i);
    REQUIRE(*v != j);

    WHEN("it is assigned an reference")
    {
      v = make_retained(j);

      REQUIRE(v == make_retained(j));
      REQUIRE(v != make_retained(i));
      REQUIRE(*v == 2);
      REQUIRE(*v == j);
      REQUIRE(*v != i);

      WHEN("the observed object is assigned a reference")
      {
        *v = i;

        REQUIRE(v == make_retained(j));
        REQUIRE(v != make_retained(i));
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

SCENARIO("`retained`s support equality and `less` comparison")
{
  int i = {};
  std::array<int, 2> is = {1, 2};

  GIVEN("an `retained` constructed from an entry in an array")
  {
    THEN("`operator==` is supported")
    {
      REQUIRE(make_retained(is[0]) == make_retained(is[0]));
      REQUIRE(!(make_retained(is[0]) == make_retained(is[1])));
      REQUIRE(!(make_retained(is[1]) == make_retained(is[0])));
    }

    THEN("`operator!=` is supported")
    {
      REQUIRE(!(make_retained(is[0]) != make_retained(is[0])));
      REQUIRE(make_retained(is[0]) != make_retained(is[1]));
      REQUIRE(make_retained(is[1]) != make_retained(is[0]));
    }

    THEN("`less` is supported")
    {
      REQUIRE(!(std::less<retained<int>>()(make_retained(is[0]), make_retained(is[0]))));
      REQUIRE(std::less<retained<int>>()(make_retained(is[0]), make_retained(is[1])));
      REQUIRE(!(std::less<retained<int>>()(make_retained(is[1]), make_retained(is[0]))));
    }
  }
}

SCENARIO("`retained`s can be created using `make_retained`")
{
  int i = {};

  GIVEN("an `retained` created with `make_retained`")
  {
    retained<int> v = make_retained(i);

    REQUIRE(v == make_retained(i));
  }
}

SCENARIO("`retained`s can be used in certain constant expressions")
{
  // !!! Not working on MSVC
  // constexpr static int const i = {};
  // constexpr retained<int const> const o = make_retained(i);
  // constexpr int j = *o;
}

SCENARIO("`retained`s can be used with STL containers")
{
  std::array<int, 3> i = {0, 1, 2};

  GIVEN("a `vector` of `retained`s")
  {
    std::vector<retained<int>> vector;

    vector.emplace_back(i[2]);
    vector.emplace_back(i[1]);
    vector.emplace_back(i[0]);

    REQUIRE(vector[0] == make_retained(i[2]));
    REQUIRE(vector[1] == make_retained(i[1]));
    REQUIRE(vector[2] == make_retained(i[0]));
  }

  GIVEN("a `map` of `retained`-`retained` pairs")
  {
    std::map<retained<int>, retained<int>> map;

    map.emplace(i[0], i[2]);
    map.emplace(i[1], i[1]);
    map.emplace(i[2], i[0]);

    REQUIRE(map.at(make_retained(i[0])) == make_retained(i[2]));
    REQUIRE(map.at(make_retained(i[1])) == make_retained(i[1]));
    REQUIRE(map.at(make_retained(i[2])) == make_retained(i[0]));
  }

  GIVEN("an `unordered_map` of `retained`-`retained` pairs")
  {
    std::unordered_map<retained<int>, retained<int>> map;

    map.emplace(i[0], i[2]);
    map.emplace(i[1], i[1]);
    map.emplace(i[2], i[0]);

    REQUIRE(map.at(make_retained(i[0])) == make_retained(i[2]));
    REQUIRE(map.at(make_retained(i[1])) == make_retained(i[1]));
    REQUIRE(map.at(make_retained(i[2])) == make_retained(i[0]));
  }

  GIVEN("a `set` of `retained`s")
  {
    std::set<retained<int>> set;

    set.emplace(i[0]);
    set.emplace(i[1]);
    set.emplace(i[2]);

    REQUIRE(set.find(make_retained(i[0])) != set.end());
    REQUIRE(set.find(make_retained(i[1])) != set.end());
    REQUIRE(set.find(make_retained(i[2])) != set.end());
  }

  GIVEN("an `unordered_set` of `retained`s")
  {
    std::unordered_set<retained<int>> set;

    set.emplace(i[0]);
    set.emplace(i[1]);
    set.emplace(i[2]);

    REQUIRE(set.find(make_retained(i[0])) != set.end());
    REQUIRE(set.find(make_retained(i[1])) != set.end());
    REQUIRE(set.find(make_retained(i[2])) != set.end());
  }
}
