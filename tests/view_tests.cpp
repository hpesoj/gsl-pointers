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

#include <propagate_const.hpp>
#include <view.hpp>

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

template <typename T>
constexpr bool is_const_v = std::is_const<T>::value;

template <typename T>
struct type_id
{
  using type = T;
};

template <typename T, typename... Ts, typename F>
void for_each_type(F f)
{
  f(type_id<T>());
  for_each_type<Ts...>(f);
}

template <typename F>
void for_each_type(F f)
{
}

template <bool B>
struct invoke_if_impl
{
    template <typename F, typename... Ts>
    void operator()(F&&, Ts&&...) const
    {
    }
};

template <>
struct invoke_if_impl<true>
{
    template <typename F, typename... Ts>
    void operator()(F&& f, Ts&&... ts) const
    {
        f(std::forward<Ts>(ts)...);
    }
};

template <bool B, typename F, typename... Ts>
void invoke_if(F&& f, Ts&&... ts)
{
    invoke_if_impl<B>()(std::forward<F>(f), std::forward<Ts>(ts)...);
}

template <typename It>
std::string strip(It b, It e)
{
    while (std::isspace(*b)) ++b;
    while (e > b + 1 && std::isspace(*(e - 1))) --e;
    return std::string(b, e);
}

std::vector<std::string> tokenize(std::string const& s)
{
    std::vector<std::string> tokens;

    auto b = s.begin();
    auto e = s.end();
    auto x = b;

    int nesting_level = {};

    for (auto i = b; i != e; ++i)
    {
        auto c = *i;

        if (c == '(') ++nesting_level;
        if (c == ')') --nesting_level;

        if ((c == ',' || i == e - 1) && nesting_level == 0)
        {
            tokens.emplace_back(strip(x, i));
            x = i + 1;
        }
    }

    return tokens;
}

#define FOR_EACH_TYPE(type_t, ...) \
    { \
        static auto _args = tokenize(#__VA_ARGS__); \
        size_t _index = {}; \
        doctest::detail::getContextState()->currentIteration.push_back(0); \
        for_each_type<__VA_ARGS__>([&](auto _id) \
        { \
            ++doctest::detail::getContextState()->currentIteration.back(); \
            DOCTEST_SUBCASE((std::string("   Using: ") + #type_t + " = " + _args[_index++]).c_str()) \
            { \
                using type_t = decltype(_id)::type;

#define NEXT_TYPE }}); doctest::detail::getContextState()->currentIteration.pop_back(); }

#define IF(condition) \
    { \
        doctest::detail::getContextState()->currentIteration.push_back(0); \
        invoke_if<condition>([&](auto) \
        { \
            {

#define END_IF }}, 0); doctest::detail::getContextState()->currentIteration.pop_back(); }

template <typename T>
struct is_view : std::false_type {};
template <typename T>
struct is_view<view<T>> : std::true_type {};

template <typename T>
constexpr bool is_view_v = is_view<T>::value;

template <typename T>
struct is_optional_view : std::false_type {};
template <typename T>
struct is_optional_view<optional_view<T>> : std::true_type {};

template <typename T>
constexpr bool is_optional_view_v = is_optional_view<T>::value;

template <typename T>
struct is_propagate_const : std::false_type {};
template <typename T>
struct is_propagate_const<propagate_const<T>> : std::true_type {};

template <typename T>
constexpr bool is_propagate_const_v = is_propagate_const<T>::value;

template <typename T, typename U>
struct replace
{
    using type = U;
};

template <typename T, typename U>
using replace_t = typename replace<T, U>::type;

template <typename T, typename U>
struct replace<T const, U>
{
    using type = replace_t<T, U> const;
};

template <typename T, typename U>
struct replace<view<T>, U>
{
    using type = view<replace_t<T, U>>;
};

template <typename T, typename U>
struct replace<optional_view<T>, U>
{
    using type = optional_view<replace_t<T, U>>;
};

template <typename T, typename U>
struct replace<propagate_const<T>, U>
{
    using type = propagate_const<replace_t<T, U>>;
};

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
    //CHECK(std::is_trivially_copyable<view<int const>>::value);
    //CHECK(std::is_trivially_copyable<optional_view<int>>::value);
    //CHECK(std::is_trivially_copyable<optional_view<int const>>::value);
}

SCENARIO("views can be constructed`")
{
    FOR_EACH_TYPE(value_t, int, int const)
    {
        FOR_EACH_TYPE(view_t, view<value_t>, optional_view<value_t>)
        {
            FOR_EACH_TYPE(final_t, view_t, propagate_const<view_t>)
            {
                value_t i = {};
                value_t j = {};

                GIVEN("a view implicitly constructed from a reference")
                {
                    final_t v = i;

                    REQUIRE(v == i);
                    REQUIRE(v != j);

                    WHEN("is is assigned a reference")
                    {
                        v = j;

                        REQUIRE(v == j);
                        REQUIRE(v != i);
                    }
                }

                GIVEN("a view explicitly constructed from a pointer")
                {
                    final_t v{&i};

                    REQUIRE(v == i);
                    REQUIRE(v != j);
                }

                GIVEN("a view explicitly constructed from a null pointer")
                {
                    IF(is_view_v<view_t>)
                    {
                        REQUIRE_THROWS(final_t v{static_cast<value_t*>(nullptr)});
                    } END_IF

                    IF(is_optional_view_v<view_t>)
                    {
                        final_t v{static_cast<value_t*>(nullptr)};

                        REQUIRE(!v);
                        REQUIRE(v != i);
                        REQUIRE(v != j);
                    } END_IF
                }

                GIVEN("a view explicitly constructed from a `nullptr`")
                {
                    IF(is_view_v<view_t>)
                    {
                        REQUIRE_THROWS(final_t v{nullptr});
                    } END_IF

                    IF(is_optional_view_v<view_t>)
                    {
                        final_t v{nullptr};

                        REQUIRE(!v);
                        REQUIRE(v != i);
                        REQUIRE(v != j);
                    } END_IF
                }

                IF(is_optional_view_v<view_t>)
                {
                    GIVEN("a default constructed view")
                    {
                        final_t v;

                        REQUIRE(!v);
                        REQUIRE(v == nullopt);

                        WHEN("it is assigned a reference")
                        {
                            v = i;

                            REQUIRE(v);
                            REQUIRE(v == i);
                            REQUIRE(v != nullopt);

                            THEN("it is assigned an empty view")
                            {
                                v = {};

                                REQUIRE(!v);
                                REQUIRE(v == nullopt);
                                REQUIRE(v != i);
                            }
                        }
                    }

                    GIVEN("a view constructed using the `{}` syntax")
                    {
                        final_t v = {};

                        REQUIRE(!v);
                        REQUIRE(v == nullopt);
                    }
                } END_IF
            } NEXT_TYPE
        } NEXT_TYPE
    } NEXT_TYPE
}

SCENARIO("views convert to references and pointers")
{
    FOR_EACH_TYPE(value_t, int, int const)
    {
        FOR_EACH_TYPE(view_t, view<value_t>, optional_view<value_t>)
        {
            FOR_EACH_TYPE(final_t, view_t, propagate_const<view_t>)
            {
                value_t i = {};

                GIVEN("a view constructed from an reference")
                {
                    final_t v = i;

                    WHEN("the view is converted to a reference")
                    {
                        IF(is_view_v<view_t>)
                        {
                            value_t& r = v;

                            REQUIRE(&r == &i);
                        } END_IF

                        IF(is_optional_view_v<view_t>)
                        {
                            value_t& r = static_cast<value_t&>(v);

                            REQUIRE(&r == &i);
                        } END_IF
                    }

                    WHEN("the view is converted to a pointer")
                    {
                        value_t* p = static_cast<value_t*>(v);

                        REQUIRE(p == &i);
                    }
                }
            } NEXT_TYPE
        } NEXT_TYPE
    } NEXT_TYPE
}

SCENARIO("views can be copied")
{
    FOR_EACH_TYPE(value_t, int, int const)
    {
        FOR_EACH_TYPE(view_t, view<value_t>, optional_view<value_t>)
        {
            FOR_EACH_TYPE(final_t, view_t, propagate_const<view_t>)
            {
                value_t i = {};
                value_t j = {};

                GIVEN("a copy constructed view")
                {
                    final_t v = i;
                    final_t w = v;

                    REQUIRE(w == v);

                    REQUIRE(w == i);
                    REQUIRE(w != j);

                    REQUIRE(v == i);
                    REQUIRE(v != j);

                    WHEN("it is copy assigned")
                    {
                        final_t x = j;
                        w = x;

                        REQUIRE(w == x);

                        REQUIRE(w == j);
                        REQUIRE(w != i);

                        REQUIRE(x == j);
                        REQUIRE(x != i);

                        REQUIRE(v == i);
                        REQUIRE(v != j);
                    }
                }
            } NEXT_TYPE
        } NEXT_TYPE
    } NEXT_TYPE
}

SCENARIO("views can be moved")
{
    FOR_EACH_TYPE(value_t, int, int const)
    {
        FOR_EACH_TYPE(view_t, view<value_t>, optional_view<value_t>)
        {
            FOR_EACH_TYPE(final_t, view_t, propagate_const<view_t>)
            {
                value_t i = {};
                value_t j = {};

                GIVEN("a move constructed view")
                {
                    final_t v = i;
                    final_t w = std::move(v);

                    REQUIRE(w == v);

                    REQUIRE(w == i);
                    REQUIRE(w != j);

                    REQUIRE(v == i);
                    REQUIRE(v != j);

                    WHEN("it is move assigned")
                    {
                        final_t x = j;
                        w = std::move(x);

                        REQUIRE(w == x);

                        REQUIRE(w == j);
                        REQUIRE(w != i);

                        REQUIRE(x == j);
                        REQUIRE(x != i);

                        REQUIRE(v == i);
                        REQUIRE(v != j);
                    }
                }
            } NEXT_TYPE
        } NEXT_TYPE
    } NEXT_TYPE
}

SCENARIO("views can be swapped")
{
    FOR_EACH_TYPE(value_t, int, int const)
    {
        FOR_EACH_TYPE(view_t, view<value_t>, optional_view<value_t>)
        {
            FOR_EACH_TYPE(final_t, view_t, propagate_const<view_t>)
            {
                value_t i = {};
                value_t j = {};

                GIVEN("a view swapped with a view")
                {
                    final_t v = i;
                    final_t w = j;

                    swap(v, w);

                    REQUIRE(v == j);
                    REQUIRE(w == i);
                }

                GIVEN("a view swapped with itself")
                {
                    final_t v = i;

                    swap(v, v);

                    REQUIRE(v == i);
                }
            } NEXT_TYPE
        } NEXT_TYPE
    } NEXT_TYPE
}

SCENARIO("views can be used to access the objects they reference")
{
    FOR_EACH_TYPE(value_t, int, int const)
    {
        FOR_EACH_TYPE(view_t, view<value_t>, optional_view<value_t>)
        {
            FOR_EACH_TYPE(final_t, view_t, propagate_const<view_t>)
            {
                value_t i = 1;
                value_t j = 2;

                GIVEN("a view constructed from an reference")
                {
                    final_t v = i;

                    REQUIRE(v == i);
                    REQUIRE(v != j);
                    REQUIRE(*v == 1);
                    REQUIRE(*v == i);
                    REQUIRE(*v != j);

                    WHEN("it is assigned an reference")
                    {
                        v = j;

                        REQUIRE(v == j);
                        REQUIRE(v != i);
                        REQUIRE(*v == 2);
                        REQUIRE(*v == j);
                        REQUIRE(*v != i);

                        IF(!is_const_v<value_t>)
                        {
                            WHEN("the viewed object is assigned a reference")
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
                        } END_IF
                    }
                }
            } NEXT_TYPE
        } NEXT_TYPE
    } NEXT_TYPE
}

SCENARIO("`view` supports arithmetic comparison")
{
    FOR_EACH_TYPE(value_t, int, int const)
    {
        FOR_EACH_TYPE(view_t, view<value_t>, optional_view<value_t>)
        {
            FOR_EACH_TYPE(final_t, view_t, propagate_const<view_t>)
            {
                std::array<value_t, 2> is = { 1, 2 };

                GIVEN("views constructed from entries in an array")
                {
                    final_t u = is[0];
                    final_t v = is[0];
                    final_t w = is[1];

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
            } NEXT_TYPE
        } NEXT_TYPE
    } NEXT_TYPE
}

SCENARIO("`view` can be static cast")
{
    //FOR_EACH_TYPE(derived_t, derived, derived const)
    //{
        //using base_t = replace_t<derived_t, base>;

        //FOR_EACH_TYPE(derived_view_t, view<derived_t>, optional_view<derived_t>)
        //{
            //using base_view_t = replace_t<derived_view_t, base>;

            //FOR_EACH_TYPE(derived_final_t, derived_view_t, propagate_const<derived_view_t>)
            //{
                //using base_final_t = replace_t<derived_final_t, base>;

                //derived_t d;

                //GIVEN("a `view<base>` initialized with `derived&`")
                //{
                    //base_final_t v = d;

                    //WHEN("it is static cast to a `view<derived>`")
                    //{
                        //base_final_t w = static_view_cast<derived>(v);

                        //REQUIRE(w == v);
                        //REQUIRE(w == d);
                    //}
                //}
            //} NEXT_TYPE
        //} NEXT_TYPE
    //} NEXT_TYPE
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

SCENARIO("`view` can be used with `propagate_const`")
{
    int i = 42;
    int j = 21;

    GIVEN("a `propagate_const<view<int>>` constructed from an `int&`")
    {
        propagate_const<view<int>> v = i;

        REQUIRE(v == i);
        REQUIRE(v != j);
        REQUIRE(*v == 42);

        WHEN("it is implicitly converted to `int&`")
        {
            int& r = v;

            REQUIRE(r == v);
            REQUIRE((std::is_same<decltype(*v), int&>::value));
        }

        WHEN("it is implicitly converted to `int const&`")
        {
            int const& r = v;

            REQUIRE(r == v);
        }

        WHEN("it is explicitly converted to `int*`")
        {
            int* p = static_cast<int*>(v);

            REQUIRE(*p == v);
        }

        WHEN("it is explicitly converted to `int const*`")
        {
            int const* p = static_cast<int const*>(v);

            REQUIRE(*p == v);
        }

        WHEN("it is converted to a pointer using `get_pointer`")
        {
            auto p = get_pointer(v);

            REQUIRE((std::is_same<decltype(p), int*>::value));
            REQUIRE(*p == v);
        }

        WHEN("it is move constructed")
        {
            propagate_const<view<int>> w = std::move(v);

            REQUIRE(w == i);
        }

        WHEN("it is implicitly copy converted to a `view<int>&`")
        {
            view<int>& w = v;

            REQUIRE(w == i);
        }

        WHEN("it is implicitly copy converted to a `view<int const>`")
        {
            view<int const> w = v;

            REQUIRE(w == i);
        }

        WHEN("it is implicitly move converted to a `view<int>`")
        {
            view<int> w = std::move(v);

            REQUIRE(w == i);
        }

        WHEN("it is implicitly move converted to a `view<int const>`")
        {
            view<int const> w = std::move(v);

            REQUIRE(w == i);
        }
    }

    GIVEN("a `propagate_const<view<int>> const` constructed from an `int&`")
    {
        propagate_const<view<int>> const v = i;

        REQUIRE(v == i);
        REQUIRE(v != j);
        REQUIRE(*v == 42);

        WHEN("it is implicitly converted to `int const&`")
        {
            int const& r = v;

            REQUIRE(r == v);
            REQUIRE((std::is_same<decltype(*v), int const&>::value));
        }

        WHEN("it is explicitly converted to `int const*`")
        {
            int const* p = static_cast<int const*>(v);

            REQUIRE(*p == v);
        }

        WHEN("it is converted to a pointer using `get_pointer`")
        {
            auto p = get_pointer(v);

            REQUIRE((std::is_same<decltype(p), int const*>::value));
            REQUIRE(*p == v);
        }

        WHEN("it is converted to a pointer using `get_pointer`")
        {
            auto p = get_pointer(v);

            REQUIRE((std::is_same<decltype(p), int const*>::value));
            REQUIRE(*p == v);
        }

        WHEN("it is implicitly copy converted to a `view<int const>`")
        {
            view<int const> w = v;

            REQUIRE(w == i);
        }

        WHEN("it is implicitly move converted to a `view<int const>`")
        {
            view<int const> w = std::move(v);

            REQUIRE(w == i);
        }
    }
}
