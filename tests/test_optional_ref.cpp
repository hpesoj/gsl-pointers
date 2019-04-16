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

#include <gsl/optional_ref.hpp>

#include <array>
#include <map>
#include <set>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace
{
  struct base
  {
    virtual ~base()
    {
    }
    virtual int get_foo() const = 0;
  };

  struct derived : base
  {
    derived() = default;

    explicit derived(int foo)
    : foo(foo)
    {
    }

    derived(derived const&) = default;
    derived(derived&& other)
    : foo(other.foo)
    {
      other.foo = 0;
    }

    int get_foo() const override
    {
      return foo;
    }

  private:
    int foo = {};
  };

  struct derived_other : base
  {
    derived_other() = default;

    explicit derived_other(int foo)
    : foo(foo)
    {
    }

    int get_foo() const override
    {
      return foo;
    }

  private:
    int foo = {};
  };

} // namespace

using gsl::make_optional_ref;
using gsl::optional_ref;

SCENARIO("`optional_ref` can be disenagaged")
{
  GIVEN("a default constructed `optional_ref`")
  {
    optional_ref<int> o;

    CHECK(!o);
    CHECK(!o.has_value());
  }

  GIVEN("an `optional_ref` constructed from `std::nullopt`")
  {
    optional_ref<int> o = std::nullopt;

    CHECK(!o);
    CHECK(!o.has_value());
  }

  GIVEN("an `optional_ref` constructed from `{}`")
  {
    optional_ref<int> o = {};

    CHECK(!o);
    CHECK(!o.has_value());
  }
}

SCENARIO("`optional_ref` can be constructed from other references")
{
  GIVEN("an `optional_ref` constructed from an `int`")
  {
    int i = {};

    optional_ref<int> o = i;

    CHECK(o);
    CHECK(o.has_value());
    CHECK(&*o == &i);

    THEN("another `optional_ref` constructed from that `optional_ref`")
    {
      optional_ref<int> p = o;

      CHECK(p);
      CHECK(p.has_value());
      CHECK(&*p == &i);
    }
  }

  GIVEN("an `optional_ref` constructed from a `derived")
  {
    derived d{42};

    optional_ref<derived> o = d;

    CHECK(o);
    CHECK(o.has_value());
    CHECK(o->get_foo() == 42);
    CHECK(&*o == &d);

    THEN("an `optional_ref` to `base` constructed from that `optional_ref`")
    {
      optional_ref<base> p = d;

      CHECK(p);
      CHECK(p.has_value());
      CHECK(o->get_foo() == 42);
      CHECK(&*p == &d);
    }
  }

  GIVEN("an `optional_ref` constructed using `make_optional_ref`")
  {
    int i = {};

    optional_ref<int> o = make_optional_ref(i);

    CHECK(o);
    CHECK(o.has_value());
    CHECK(&*o == &i);
  }
}

SCENARIO("`optional_ref` can be accessed via the throwing `value` function")
{
  GIVEN("an `optional_ref` constructed from an lvalue reference")
  {
    int i = {};

    optional_ref<int> o = i;

    CHECK(o.value() == i);
    CHECK(&o.value() == &i);
  }

  GIVEN("a default constructed `optional_ref`")
  {
    optional_ref<int> o;

    CHECK_THROWS(o.value());
  }
}

SCENARIO(
    "`optional_ref` can be conditionally accessed through the `value_or` "
    "function")
{
  GIVEN("an `optional_ref` constructed from an lvalue reference")
  {
    int i = {};

    optional_ref<int> o = i;

    CHECK(o.value_or(42) == i);
  }

  GIVEN("a default constructed `optional_ref`")
  {
    optional_ref<int> o;

    CHECK(o.value_or(42) == 42);
  }
}

SCENARIO("`optional_ref` can be arithmetically compared")
{
  GIVEN("`a = 0`, `b = 1`, `x = a`, `y = b` and `o = std::nullopt`")
  {
    int a = 0;
    int b = 1;

    optional_ref<int> x = a;
    optional_ref<int> y = b;
    optional_ref<int> o = std::nullopt;

    CHECK(x);
    CHECK(y);
    CHECK(!o);

    CHECK((x == x));
    CHECK((o == o));
    CHECK(!(x == o));
    CHECK(!(o == x));
    CHECK(!(x == y));
    CHECK(!(y == x));
    CHECK(!(x == std::nullopt));
    CHECK(!(std::nullopt == x));
    CHECK((o == std::nullopt));
    CHECK((std::nullopt == o));
    CHECK((x == a));
    CHECK((a == x));
    CHECK(!(x == b));
    CHECK(!(b == x));
    CHECK(!(y == a));
    CHECK(!(a == y));
    CHECK((y == b));
    CHECK((b == y));
    CHECK(!(o == a));
    CHECK(!(a == o));

    CHECK(!(x != x));
    CHECK(!(o != o));
    CHECK((x != o));
    CHECK((o != x));
    CHECK((x != y));
    CHECK((y != x));
    CHECK((x != std::nullopt));
    CHECK((std::nullopt != x));
    CHECK(!(o != std::nullopt));
    CHECK(!(std::nullopt != o));
    CHECK(!(x != a));
    CHECK(!(a != x));
    CHECK((x != b));
    CHECK((b != x));
    CHECK((y != a));
    CHECK((a != y));
    CHECK(!(y != b));
    CHECK(!(b != y));
    CHECK((o != a));
    CHECK((a != o));

    CHECK(!(x < x));
    CHECK(!(o < o));
    CHECK(!(x < o));
    CHECK((o < x));
    CHECK((x < y));
    CHECK(!(y < x));
    CHECK(!(x < std::nullopt));
    CHECK((std::nullopt < x));
    CHECK(!(o < std::nullopt));
    CHECK(!(std::nullopt < o));
    CHECK(!(x < a));
    CHECK(!(a < x));
    CHECK((x < b));
    CHECK(!(b < x));
    CHECK(!(y < a));
    CHECK((a < y));
    CHECK(!(y < b));
    CHECK(!(b < y));
    CHECK((o < a));
    CHECK(!(a < o));

    CHECK((x <= x));
    CHECK((o <= o));
    CHECK(!(x <= o));
    CHECK((o <= x));
    CHECK((x <= y));
    CHECK(!(y <= x));
    CHECK(!(x <= std::nullopt));
    CHECK((std::nullopt <= x));
    CHECK((o <= std::nullopt));
    CHECK((std::nullopt <= o));
    CHECK((x <= a));
    CHECK((a <= x));
    CHECK((x <= b));
    CHECK(!(b <= x));
    CHECK(!(y <= a));
    CHECK((a <= y));
    CHECK((y <= b));
    CHECK((b <= y));
    CHECK((o <= a));
    CHECK(!(a <= o));

    CHECK(!(x > x));
    CHECK(!(o > o));
    CHECK((x > o));
    CHECK(!(o > x));
    CHECK(!(x > y));
    CHECK((y > x));
    CHECK((x > std::nullopt));
    CHECK(!(std::nullopt > x));
    CHECK(!(o > std::nullopt));
    CHECK(!(std::nullopt > o));
    CHECK(!(x > a));
    CHECK(!(a > x));
    CHECK(!(x > b));
    CHECK((b > x));
    CHECK((y > a));
    CHECK(!(a > y));
    CHECK(!(y > b));
    CHECK(!(b > y));
    CHECK(!(o > a));
    CHECK((a > o));

    CHECK((x >= x));
    CHECK((o >= o));
    CHECK((x >= o));
    CHECK(!(o >= x));
    CHECK(!(x >= y));
    CHECK((y >= x));
    CHECK((x >= std::nullopt));
    CHECK(!(std::nullopt >= x));
    CHECK((o >= std::nullopt));
    CHECK((std::nullopt >= o));
    CHECK((x >= a));
    CHECK((a >= x));
    CHECK(!(x >= b));
    CHECK((b >= x));
    CHECK((y >= a));
    CHECK(!(a >= y));
    CHECK((y >= b));
    CHECK((b >= y));
    CHECK(!(o >= a));
    CHECK((a >= o));
  }
}

SCENARIO("`optional_ref`s can be used with STL containers")
{
  std::array<int, 3> i = {0, 1, 2};

  GIVEN("a `vector` of `optional_ref`s")
  {
    std::vector<optional_ref<int>> vector;

    vector.emplace_back(i[2]);
    vector.emplace_back(i[1]);
    vector.emplace_back(i[0]);

    REQUIRE(vector[0] == make_optional_ref(i[2]));
    REQUIRE(vector[1] == make_optional_ref(i[1]));
    REQUIRE(vector[2] == make_optional_ref(i[0]));
  }

  GIVEN("a `map` of `optional_ref`-`optional_ref` pairs")
  {
    std::map<optional_ref<int>, optional_ref<int>> map;

    map.emplace(i[0], i[2]);
    map.emplace(i[1], i[1]);
    map.emplace(i[2], i[0]);

    REQUIRE(map.at(make_optional_ref(i[0])) == make_optional_ref(i[2]));
    REQUIRE(map.at(make_optional_ref(i[1])) == make_optional_ref(i[1]));
    REQUIRE(map.at(make_optional_ref(i[2])) == make_optional_ref(i[0]));
  }

  GIVEN("an `unordered_map` of `optional_ref`-`optional_ref` pairs")
  {
    std::unordered_map<optional_ref<int>, optional_ref<int>> map;

    map.emplace(i[0], i[2]);
    map.emplace(i[1], i[1]);
    map.emplace(i[2], i[0]);

    REQUIRE(map.at(make_optional_ref(i[0])) == make_optional_ref(i[2]));
    REQUIRE(map.at(make_optional_ref(i[1])) == make_optional_ref(i[1]));
    REQUIRE(map.at(make_optional_ref(i[2])) == make_optional_ref(i[0]));
  }

  GIVEN("a `set` of `optional_ref`s")
  {
    std::set<optional_ref<int>> set;

    set.emplace(i[0]);
    set.emplace(i[1]);
    set.emplace(i[2]);

    REQUIRE(set.find(make_optional_ref(i[0])) != set.end());
    REQUIRE(set.find(make_optional_ref(i[1])) != set.end());
    REQUIRE(set.find(make_optional_ref(i[2])) != set.end());
  }

  GIVEN("an `unordered_set` of `optional_ref`s")
  {
    std::unordered_set<optional_ref<int>> set;

    set.emplace(i[0]);
    set.emplace(i[1]);
    set.emplace(i[2]);

    REQUIRE(set.find(make_optional_ref(i[0])) != set.end());
    REQUIRE(set.find(make_optional_ref(i[1])) != set.end());
    REQUIRE(set.find(make_optional_ref(i[2])) != set.end());
  }
}
