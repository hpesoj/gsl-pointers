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

#include <indirect.hpp>
#include <propagate_const.hpp>

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
struct is_indirect : std::false_type {};
template <typename T>
struct is_indirect<indirect<T>> : std::true_type {};

template <typename T>
constexpr bool is_indirect_v = is_indirect<T>::value;

template <typename T>
struct is_optional_indirect : std::false_type {};
template <typename T>
struct is_optional_indirect<optional_indirect<T>> : std::true_type {};

template <typename T>
constexpr bool is_optional_indirect_v = is_optional_indirect<T>::value;

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
struct replace<indirect<T>, U>
{
    using type = indirect<replace_t<T, U>>;
};

template <typename T, typename U>
struct replace<optional_indirect<T>, U>
{
    using type = optional_indirect<replace_t<T, U>>;
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

SCENARIO("`indirect` and `optional_indirect` are trivially copyable")
{
    // !!! Not working on MSVC
    //CHECK(std::is_trivially_copyable<indirect<int>>::value);
    //CHECK(std::is_trivially_copyable<indirect<int const>>::value);
    //CHECK(std::is_trivially_copyable<optional_indirect<int>>::value);
    //CHECK(std::is_trivially_copyable<optional_indirect<int const>>::value);
}

SCENARIO("indirects can be constructed`")
{
    FOR_EACH_TYPE(value_t, int, int const)
    {
        FOR_EACH_TYPE(indirect_t, indirect<value_t>, optional_indirect<value_t>)
        {
            FOR_EACH_TYPE(final_t, indirect_t, propagate_const<indirect_t>)
            {
                value_t i = {};
                value_t j = {};

                GIVEN("a indirect implicitly constructed from a reference")
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

                GIVEN("a indirect explicitly constructed from a pointer")
                {
                    final_t v{&i};

                    REQUIRE(v == i);
                    REQUIRE(v != j);
                }

                GIVEN("a indirect explicitly constructed from a null pointer")
                {
                    IF(is_indirect_v<indirect_t>)
                    {
                        REQUIRE_THROWS(final_t v{static_cast<value_t*>(nullptr)});
                    } END_IF

                    IF(is_optional_indirect_v<indirect_t>)
                    {
                        final_t v{static_cast<value_t*>(nullptr)};

                        REQUIRE(!v);
                        REQUIRE(v != i);
                        REQUIRE(v != j);
                    } END_IF
                }

                GIVEN("a indirect explicitly constructed from a `nullptr`")
                {
                    IF(is_indirect_v<indirect_t>)
                    {
                        REQUIRE_THROWS(final_t v{nullptr});
                    } END_IF

                    IF(is_optional_indirect_v<indirect_t>)
                    {
                        final_t v{nullptr};

                        REQUIRE(!v);
                        REQUIRE(v != i);
                        REQUIRE(v != j);
                    } END_IF
                }

                IF(is_optional_indirect_v<indirect_t>)
                {
                    GIVEN("a default constructed indirect")
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

                            THEN("it is assigned an empty indirect")
                            {
                                v = {};

                                REQUIRE(!v);
                                REQUIRE(v == nullopt);
                                REQUIRE(v != i);
                            }
                        }
                    }

                    GIVEN("a indirect constructed using the `{}` syntax")
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

SCENARIO("indirects convert to references and pointers")
{
    FOR_EACH_TYPE(value_t, int, int const)
    {
        FOR_EACH_TYPE(indirect_t, indirect<value_t>, optional_indirect<value_t>)
        {
            FOR_EACH_TYPE(final_t, indirect_t, propagate_const<indirect_t>)
            {
                value_t i = {};

                GIVEN("a indirect constructed from an reference")
                {
                    final_t v = i;

                    WHEN("the indirect is converted to a reference")
                    {
                        IF(is_indirect_v<indirect_t>)
                        {
                            value_t& r = v;

                            REQUIRE(&r == &i);
                        } END_IF

                        IF(is_optional_indirect_v<indirect_t>)
                        {
                            value_t& r = static_cast<value_t&>(v);

                            REQUIRE(&r == &i);
                        } END_IF
                    }

                    WHEN("the indirect is converted to a pointer")
                    {
                        value_t* p = static_cast<value_t*>(v);

                        REQUIRE(p == &i);
                    }
                }
            } NEXT_TYPE
        } NEXT_TYPE
    } NEXT_TYPE
}

SCENARIO("indirects can be copied")
{
    FOR_EACH_TYPE(value_t, int, int const)
    {
        FOR_EACH_TYPE(indirect_t, indirect<value_t>, optional_indirect<value_t>)
        {
            FOR_EACH_TYPE(final_t, indirect_t, propagate_const<indirect_t>)
            {
                value_t i = {};
                value_t j = {};

                GIVEN("a copy constructed indirect")
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

SCENARIO("indirects can be moved")
{
    FOR_EACH_TYPE(value_t, int, int const)
    {
        FOR_EACH_TYPE(indirect_t, indirect<value_t>, optional_indirect<value_t>)
        {
            FOR_EACH_TYPE(final_t, indirect_t, propagate_const<indirect_t>)
            {
                value_t i = {};
                value_t j = {};

                GIVEN("a move constructed indirect")
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

SCENARIO("indirects can be swapped")
{
    FOR_EACH_TYPE(value_t, int, int const)
    {
        FOR_EACH_TYPE(indirect_t, indirect<value_t>, optional_indirect<value_t>)
        {
            FOR_EACH_TYPE(final_t, indirect_t, propagate_const<indirect_t>)
            {
                value_t i = {};
                value_t j = {};

                GIVEN("a indirect swapped with a indirect")
                {
                    final_t v = i;
                    final_t w = j;

                    swap(v, w);

                    REQUIRE(v == j);
                    REQUIRE(w == i);
                }

                GIVEN("a indirect swapped with itself")
                {
                    final_t v = i;

                    swap(v, v);

                    REQUIRE(v == i);
                }
            } NEXT_TYPE
        } NEXT_TYPE
    } NEXT_TYPE
}

SCENARIO("indirects can be used to access the objects they reference")
{
    FOR_EACH_TYPE(value_t, int, int const)
    {
        FOR_EACH_TYPE(indirect_t, indirect<value_t>, optional_indirect<value_t>)
        {
            FOR_EACH_TYPE(final_t, indirect_t, propagate_const<indirect_t>)
            {
                value_t i = 1;
                value_t j = 2;

                GIVEN("a indirect constructed from an reference")
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
                            WHEN("the indirected object is assigned a reference")
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

SCENARIO("indirects support arithmetic comparison")
{
    FOR_EACH_TYPE(value_t, int, int const)
    {
        FOR_EACH_TYPE(indirect_t, indirect<value_t>, optional_indirect<value_t>)
        {
            FOR_EACH_TYPE(final_t, indirect_t, propagate_const<indirect_t>)
            {
                std::array<value_t, 2> is = { 1, 2 };

                GIVEN("indirects constructed from entries in an array")
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

SCENARIO("indirects can be static cast")
{
    FOR_EACH_TYPE(derived_t, derived, derived const)
    {
        using base_t = replace_t<derived_t, base>;

        FOR_EACH_TYPE(derived_indirect_t, indirect<derived_t>, optional_indirect<derived_t>)
        {
            using base_indirect_t = replace_t<derived_indirect_t, base>;

            FOR_EACH_TYPE(derived_final_t, derived_indirect_t/*, propagate_const<derived_indirect_t>*/)
            {
                using base_final_t = replace_t<derived_final_t, base>;

                derived_t d;

                GIVEN("a indirect to `base` initialized with a `derived`")
                {
                    base_final_t v = d;

                    WHEN("it is static cast to a indirect to `derived`")
                    {
                        derived_final_t w = static_indirect_cast<derived_t>(v);

                        REQUIRE(w == v);
                        REQUIRE(w == d);
                    }
                }
            } NEXT_TYPE
        } NEXT_TYPE
    } NEXT_TYPE
}

SCENARIO("indirects can be dynamic cast")
{
    FOR_EACH_TYPE(derived_t, derived, derived const)
    {
        using base_t = replace_t<derived_t, base>;
        using derived_other_t = replace_t<derived_t, derived_other>;

        FOR_EACH_TYPE(derived_indirect_t, indirect<derived_t>, optional_indirect<derived_t>)
        {
            using base_indirect_t = replace_t<derived_indirect_t, base>;

            FOR_EACH_TYPE(derived_final_t, derived_indirect_t/*, propagate_const<derived_indirect_t>*/)
            {
                using base_final_t = replace_t<derived_final_t, base>;

                derived_t d;
                derived_other_t e;

                GIVEN("a indirect to `base` initialized with a `derived`")
                {
                    base_final_t v = d;

                    WHEN("it is dynamic cast to a indirect to `derived`")
                    {
                        derived_final_t w = dynamic_indirect_cast<derived_t>(v);

                        REQUIRE(w == v);
                        REQUIRE(w == d);
                    }
                }

                GIVEN("a indirect to `base` initialized with a `derived_other`")
                {
                    base_final_t v = e;

                    WHEN("it is dynamic cast to a indirect to `derived`")
                    {
                        IF(is_indirect_v<base_indirect_t>)
                        {
                            REQUIRE_THROWS(derived_final_t w = dynamic_indirect_cast<derived_t>(v));
                        } END_IF

                        IF(is_optional_indirect_v<base_indirect_t>)
                        {
                            derived_final_t w = dynamic_indirect_cast<derived_t>(v);

                            REQUIRE(!w);
                            REQUIRE(w != v);
                            REQUIRE(w != d);
                        } END_IF
                    }
                }
            } NEXT_TYPE
        } NEXT_TYPE
    } NEXT_TYPE
}

SCENARIO("indirects can be const cast")
{
    FOR_EACH_TYPE(value_t, int, int const)
    {
        using const_value_t = std::add_const_t<value_t>;

        FOR_EACH_TYPE(indirect_t, indirect<value_t>, optional_indirect<value_t>)
        {
            using const_indirect_t = replace_t<indirect_t, const_value_t>;

            FOR_EACH_TYPE(final_t, indirect_t/*, propagate_const<indirect_t>*/)
            {
                using const_final_t = replace_t<final_t, const_value_t>;

                value_t i = {};

                GIVEN("a indirect to const")
                {
                    const_final_t v = i;

                    WHEN("it is const cast to non-const`")
                    {
                        final_t w = const_indirect_cast<value_t>(v);

                        REQUIRE(w == v);
                    }
                }
            } NEXT_TYPE
        } NEXT_TYPE
    } NEXT_TYPE
}

SCENARIO("indirects can be created using `make_indirect`")
{
    FOR_EACH_TYPE(value_t, int, int const)
    {
        FOR_EACH_TYPE(indirect_t, indirect<value_t>, optional_indirect<value_t>)
        {
            FOR_EACH_TYPE(final_t, indirect_t, propagate_const<indirect_t>)
            {
                value_t i = {};

                GIVEN("a indirect created with `make_indirect`")
                {
                    final_t v = make_indirect(i);

                    REQUIRE(v == i);
                }
            } NEXT_TYPE
        } NEXT_TYPE
    } NEXT_TYPE
}

SCENARIO("indirects can be used in certain constant expressions")
{
    // !!! Not working on MSVC
    //constexpr static derived d = { 0 };

    //constexpr indirect<foo const> v = d;
    //constexpr indirect<foo const> w = v;

    //constexpr int const& b = v->foo;
    //constexpr derived const& r1 = *v;
    //constexpr derived const& r2 = v.value();
}

SCENARIO("indirects can be used with STL containers")
{
    FOR_EACH_TYPE(value_t, int, int const)
    {
        FOR_EACH_TYPE(indirect_t, indirect<value_t>, optional_indirect<value_t>)
        {
            FOR_EACH_TYPE(final_t, indirect_t, propagate_const<indirect_t>)
            {
                std::array<value_t, 3> i = { 0, 1, 2 };

                // !!! Not working on MSVC
                //GIVEN("a `vector` of indirects")
                //{
                    //std::vector<final_t> vector;

                    //vector.emplace_back(i[2]);
                    //vector.emplace_back(i[1]);
                    //vector.emplace_back(i[0]);

                    //REQUIRE(vector[0] == i[2]);
                    //REQUIRE(vector[1] == i[1]);
                    //REQUIRE(vector[2] == i[0]);
                //}

                GIVEN("a `map` of indirect-indirect pairs")
                {
                    std::map<final_t, final_t> map;

                    map.emplace(i[0], i[2]);
                    map.emplace(i[1], i[1]);
                    map.emplace(i[2], i[0]);

                    REQUIRE(map.at(i[0]) == i[2]);
                    REQUIRE(map.at(i[1]) == i[1]);
                    REQUIRE(map.at(i[2]) == i[0]);
                }

                GIVEN("an `unordered_map` of indirect-indirect pairs")
                {
                    std::unordered_map<final_t, final_t> map;

                    map.emplace(i[0], i[2]);
                    map.emplace(i[1], i[1]);
                    map.emplace(i[2], i[0]);

                    REQUIRE(map.at(i[0]) == i[2]);
                    REQUIRE(map.at(i[1]) == i[1]);
                    REQUIRE(map.at(i[2]) == i[0]);
                }

                GIVEN("a `set` of indirects")
                {
                    std::set<final_t> set;

                    set.emplace(i[0]);
                    set.emplace(i[1]);
                    set.emplace(i[2]);

                    REQUIRE(set.find(i[0]) != set.end());
                    REQUIRE(set.find(i[1]) != set.end());
                    REQUIRE(set.find(i[2]) != set.end());
                }

                GIVEN("an `unordered_set` of indirects")
                {
                    std::unordered_set<final_t> set;

                    set.emplace(i[0]);
                    set.emplace(i[1]);
                    set.emplace(i[2]);

                    REQUIRE(set.find(i[0]) != set.end());
                    REQUIRE(set.find(i[1]) != set.end());
                    REQUIRE(set.find(i[2]) != set.end());
                }
            } NEXT_TYPE
        } NEXT_TYPE
    } NEXT_TYPE
}

class node
{
private:
    optional_indirect<node> parent;
    std::vector<indirect<node>> children;

public:
    node() = default;

    node(node const&) = delete;
    node& operator=(node const&) = delete;

    node(node&&) = delete;
    node& operator=(node&&) = delete;

    void set_parent(optional_indirect<node> new_parent) {
        if (parent) parent->remove_child(*this);
        parent = new_parent;
        if (parent) parent->add_child(*this);
    }

    optional_indirect<node> get_parent() const {
        return parent;
    }

    std::size_t get_child_count() const {
        return children.size();
    }

    indirect<node> get_child(std::size_t index) const {
        return children[index];
    }

private:
    void add_child(indirect<node> child) {
        children.push_back(child);
    }

    void remove_child(indirect<node> child) {
        children.erase(std::find(children.begin(), children.end(), child));
    }
};
