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
struct is_observer : std::false_type {};
template <typename T>
struct is_observer<observer<T>> : std::true_type {};

template <typename T>
constexpr bool is_observer_v = is_observer<T>::value;

template <typename T>
struct is_observer_ptr : std::false_type {};
template <typename T>
struct is_observer_ptr<observer_ptr<T>> : std::true_type {};

template <typename T>
constexpr bool is_observer_ptr_v = is_observer_ptr<T>::value;

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
struct replace<observer<T>, U>
{
    using type = observer<replace_t<T, U>>;
};

template <typename T, typename U>
struct replace<observer_ptr<T>, U>
{
    using type = observer_ptr<replace_t<T, U>>;
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

SCENARIO("`observer` and `observer_ptr` are trivially copyable")
{
    // !!! Not working on MSVC
    //CHECK(std::is_trivially_copyable<observer<int>>::value);
    //CHECK(std::is_trivially_copyable<observer<int const>>::value);
    //CHECK(std::is_trivially_copyable<observer_ptr<int>>::value);
    //CHECK(std::is_trivially_copyable<observer_ptr<int const>>::value);
}

SCENARIO("observers can be constructed")
{
    FOR_EACH_TYPE(value_t, int, int const)
    {
        FOR_EACH_TYPE(observer_t, observer<value_t>, observer_ptr<value_t>)
        {
            FOR_EACH_TYPE(final_t, observer_t, propagate_const<observer_t>)
            {
                value_t i = {};
                value_t j = {};

                GIVEN("a observer implicitly constructed from a reference")
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

                IF(is_observer_ptr_v<observer_t>)
                {
                    GIVEN("a default constructed observer")
                    {
                        final_t v;

                        REQUIRE(!v);
                        REQUIRE(v == nullptr);

                        WHEN("it is assigned a reference")
                        {
                            v = i;

                            REQUIRE(v);
                            REQUIRE(v == i);
                            REQUIRE(v != nullptr);

                            THEN("it is assigned an empty observer")
                            {
                                v = {};

                                REQUIRE(!v);
                                REQUIRE(v == nullptr);
                                REQUIRE(v != i);
                            }
                        }
                    }

                    GIVEN("a observer constructed using the `{}` syntax")
                    {
                        final_t v = {};

                        REQUIRE(!v);
                        REQUIRE(v == nullptr);
                    }
                } END_IF
            } NEXT_TYPE
        } NEXT_TYPE
    } NEXT_TYPE
}

SCENARIO("observers convert to pointers")
{
    FOR_EACH_TYPE(value_t, int, int const)
    {
        FOR_EACH_TYPE(observer_t, observer<value_t>, observer_ptr<value_t>)
        {
            FOR_EACH_TYPE(final_t, observer_t, propagate_const<observer_t>)
            {
                value_t i = {};

                GIVEN("a observer constructed from an reference")
                {
                    final_t v = i;

                    WHEN("the observer is converted to a pointer")
                    {
                        value_t* p = static_cast<value_t*>(v);

                        REQUIRE(p == &i);
                    }
                }
            } NEXT_TYPE
        } NEXT_TYPE
    } NEXT_TYPE
}

SCENARIO("observers can be copied")
{
    FOR_EACH_TYPE(value_t, int, int const)
    {
        FOR_EACH_TYPE(observer_t, observer<value_t>, observer_ptr<value_t>)
        {
            FOR_EACH_TYPE(final_t, observer_t, propagate_const<observer_t>)
            {
                value_t i = {};
                value_t j = {};

                GIVEN("a copy constructed observer")
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

SCENARIO("observers can be moved")
{
    FOR_EACH_TYPE(value_t, int, int const)
    {
        FOR_EACH_TYPE(observer_t, observer<value_t>, observer_ptr<value_t>)
        {
            FOR_EACH_TYPE(final_t, observer_t, propagate_const<observer_t>)
            {
                value_t i = {};
                value_t j = {};

                GIVEN("a move constructed observer")
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

SCENARIO("observers can be swapped")
{
    FOR_EACH_TYPE(value_t, int, int const)
    {
        FOR_EACH_TYPE(observer_t, observer<value_t>, observer_ptr<value_t>)
        {
            FOR_EACH_TYPE(final_t, observer_t, propagate_const<observer_t>)
            {
                value_t i = {};
                value_t j = {};

                GIVEN("a observer swapped with a observer")
                {
                    final_t v = i;
                    final_t w = j;

                    swap(v, w);

                    REQUIRE(v == j);
                    REQUIRE(w == i);
                }

                GIVEN("a observer swapped with itself")
                {
                    final_t v = i;

                    swap(v, v);

                    REQUIRE(v == i);
                }
            } NEXT_TYPE
        } NEXT_TYPE
    } NEXT_TYPE
}

SCENARIO("observers can be used to access the objects they reference")
{
    FOR_EACH_TYPE(value_t, int, int const)
    {
        FOR_EACH_TYPE(observer_t, observer<value_t>, observer_ptr<value_t>)
        {
            FOR_EACH_TYPE(final_t, observer_t, propagate_const<observer_t>)
            {
                value_t i = 1;
                value_t j = 2;

                GIVEN("a observer constructed from an reference")
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
                            WHEN("the observered object is assigned a reference")
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

SCENARIO("observers support arithmetic comparison")
{
    FOR_EACH_TYPE(value_t, int, int const)
    {
        FOR_EACH_TYPE(observer_t, observer<value_t>, observer_ptr<value_t>)
        {
            FOR_EACH_TYPE(final_t, observer_t, propagate_const<observer_t>)
            {
                value_t i = {};
                std::array<value_t, 2> is = { 1, 2 };

                final_t v = is[0];

                GIVEN("an observer constructed from an entry in an array")
                {
                    THEN("`operator==` is supported")
                    {
                        REQUIRE(v == is[0]);
                        REQUIRE(is[0] == v);
                        REQUIRE(!(v == is[1]));
                        REQUIRE(!(is[1] == v));
                    }

                    THEN("`operator!=` is supported")
                    {
                        REQUIRE(!(v != is[0]));
                        REQUIRE(!(is[0] != v));
                        REQUIRE(v != is[1]);
                        REQUIRE(is[1] != v);
                    }

                    THEN("`operator<` is supported")
                    {
                        REQUIRE(!(v < is[0]));
                        REQUIRE(!(is[0] < v));
                        REQUIRE(v < is[1]);
                        REQUIRE(!(is[1] < v));
                    }

                    THEN("`operator<=` is supported")
                    {
                        REQUIRE(v <= is[0]);
                        REQUIRE(is[0] <= v);
                        REQUIRE(v <= is[1]);
                        REQUIRE(!(is[1] <= v));
                    }

                    THEN("`operator>` is supported")
                    {
                        REQUIRE(!(v > is[0]));
                        REQUIRE(!(is[0] > v));
                        REQUIRE(!(v > is[1]));
                        REQUIRE(is[1] > v);
                    }

                    THEN("`operator>=` is supported")
                    {
                        REQUIRE(v >= is[0]);
                        REQUIRE(is[0] >= v);
                        REQUIRE(!(v >= is[1]));
                        REQUIRE(is[1] >= v);
                    }
                }

                IF(is_observer_ptr_v<observer_t>)
                {
                    GIVEN("a non-null observer and a null observer")
                    {
                        final_t v = i;
                        final_t u;

                        THEN("`operator==` with `nullptr` is supported")
                        {
                            REQUIRE(!(v == nullptr));
                            REQUIRE(!(nullptr == v));
                            REQUIRE(u == nullptr);
                            REQUIRE(nullptr == u);
                        }

                        THEN("`operator!=` with `nullptr` is supported")
                        {
                            REQUIRE(v != nullptr);
                            REQUIRE(nullptr != v);
                            REQUIRE(!(u != nullptr));
                            REQUIRE(!(nullptr != u));
                        }

                        THEN("`operator<` with `nullptr` is supported")
                        {
                            REQUIRE((v < nullptr) == std::less<value_t*>()(get_pointer(v), nullptr));
                            REQUIRE((nullptr < v) == std::less<value_t*>()(nullptr, get_pointer(v)));
                            REQUIRE(!(u < nullptr));
                            REQUIRE(!(nullptr < u));
                        }

                        THEN("`operator>` with `nullptr` is supported")
                        {
                            REQUIRE((v > nullptr) == std::greater<value_t*>()(get_pointer(v), nullptr));
                            REQUIRE((nullptr > v) == std::greater<value_t*>()(nullptr, get_pointer(v)));
                            REQUIRE(!(u > nullptr));
                            REQUIRE(!(nullptr > u));
                        }

                        THEN("`operator<=` with `nullptr` is supported")
                        {
                            REQUIRE((v <= nullptr) == std::less_equal<value_t*>()(get_pointer(v), nullptr));
                            REQUIRE((nullptr <= v) == std::less_equal<value_t*>()(nullptr, get_pointer(v)));
                            REQUIRE(u <= nullptr);
                            REQUIRE(nullptr <= u);
                        }

                        THEN("`operator>=` with `nullptr` is supported")
                        {
                            REQUIRE((v >= nullptr) == std::greater_equal<value_t*>()(get_pointer(v), nullptr));
                            REQUIRE((nullptr >= v) == std::greater_equal<value_t*>()(nullptr, get_pointer(v)));
                            REQUIRE(u >= nullptr);
                            REQUIRE(nullptr >= u);
                        }
                    }
                } END_IF

                FOR_EACH_TYPE(observer2_t, observer<value_t>, observer_ptr<value_t>)
                {
                    FOR_EACH_TYPE(final2_t, observer2_t, propagate_const<observer2_t>)
                    {
                        GIVEN("observers constructed from entries in an array")
                        {
                            final2_t u = is[0];
                            final2_t w = is[1];

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
                    } NEXT_TYPE
                } NEXT_TYPE
            } NEXT_TYPE
        } NEXT_TYPE
    } NEXT_TYPE
}

SCENARIO("observers can be created using `make_observer`")
{
    FOR_EACH_TYPE(value_t, int, int const)
    {
        FOR_EACH_TYPE(observer_t, observer<value_t>, observer_ptr<value_t>)
        {
            FOR_EACH_TYPE(final_t, observer_t, propagate_const<observer_t>)
            {
                value_t i = {};

                GIVEN("a observer created with `make_observer`")
                {
                    final_t v = make_observer(i);

                    REQUIRE(v == i);
                }
            } NEXT_TYPE
        } NEXT_TYPE
    } NEXT_TYPE
}

SCENARIO("observers can be used in certain constant expressions")
{
    // !!! Not working on MSVC
    //constexpr static derived d = { 0 };

    //constexpr observer<foo const> v = d;
    //constexpr observer<foo const> w = v;

    //constexpr int const& b = v->foo;
    //constexpr derived const& r1 = *v;
    //constexpr derived const& r2 = v.value();
}

SCENARIO("observers can be used with STL containers")
{
    FOR_EACH_TYPE(value_t, int, int const)
    {
        FOR_EACH_TYPE(observer_t, observer<value_t>, observer_ptr<value_t>)
        {
            FOR_EACH_TYPE(final_t, observer_t, propagate_const<observer_t>)
            {
                std::array<value_t, 3> i = { 0, 1, 2 };

                // !!! Not working on MSVC
                //GIVEN("a `vector` of observers")
                //{
                    //std::vector<final_t> vector;

                    //vector.emplace_back(i[2]);
                    //vector.emplace_back(i[1]);
                    //vector.emplace_back(i[0]);

                    //REQUIRE(vector[0] == i[2]);
                    //REQUIRE(vector[1] == i[1]);
                    //REQUIRE(vector[2] == i[0]);
                //}

                GIVEN("a `map` of observer-observer pairs")
                {
                    std::map<final_t, final_t> map;

                    map.emplace(i[0], i[2]);
                    map.emplace(i[1], i[1]);
                    map.emplace(i[2], i[0]);

                    REQUIRE(map.at(i[0]) == i[2]);
                    REQUIRE(map.at(i[1]) == i[1]);
                    REQUIRE(map.at(i[2]) == i[0]);
                }

                GIVEN("an `unordered_map` of observer-observer pairs")
                {
                    std::unordered_map<final_t, final_t> map;

                    map.emplace(i[0], i[2]);
                    map.emplace(i[1], i[1]);
                    map.emplace(i[2], i[0]);

                    REQUIRE(map.at(i[0]) == i[2]);
                    REQUIRE(map.at(i[1]) == i[1]);
                    REQUIRE(map.at(i[2]) == i[0]);
                }

                GIVEN("a `set` of observers")
                {
                    std::set<final_t> set;

                    set.emplace(i[0]);
                    set.emplace(i[1]);
                    set.emplace(i[2]);

                    REQUIRE(set.find(i[0]) != set.end());
                    REQUIRE(set.find(i[1]) != set.end());
                    REQUIRE(set.find(i[2]) != set.end());
                }

                GIVEN("an `unordered_set` of observers")
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
    observer_ptr<node> parent;
    std::vector<observer<node>> children;

public:
    node() = default;

    node(node const&) = delete;
    node& operator=(node const&) = delete;

    node(node&&) = delete;
    node& operator=(node&&) = delete;

    void set_parent(observer_ptr<node> new_parent) {
        if (parent) parent->remove_child(*this);
        parent = new_parent;
        if (parent) parent->add_child(*this);
    }

    observer_ptr<node> get_parent() const {
        return parent;
    }

    std::size_t get_child_count() const {
        return children.size();
    }

    observer<node> get_child(std::size_t index) const {
        return children[index];
    }

private:
    void add_child(observer<node> child) {
        children.push_back(child);
    }

    void remove_child(observer<node> child) {
        children.erase(std::find(children.begin(), children.end(), child));
    }
};
