#include <gtest/gtest.h>
#include "common/pack_sort.hpp"

using namespace rn;

struct TypeA
{};

struct TypeB
{};

struct TypeC
{};

template <typename... Args>
struct TestContainer
{};

TEST(PackSortTests, CanSortTypePack)
{
    static_assert(std::is_same_v<
        meta::PackSort<meta::ArgList<TypeA, TypeB>>::type,
        meta::PackSort<meta::ArgList<TypeB, TypeA>>::type>);

    static_assert(std::is_same_v<
        meta::PackSort<meta::ArgList<TypeA>>::type,
        meta::PackSort<meta::ArgList<TypeA>>::type>);

    static_assert(!std::is_same_v<
        meta::PackSort<meta::ArgList<TypeA>>::type,
        meta::PackSort<meta::ArgList<TypeB>>::type>);

    using C0 = meta::InstantiateSortedT<TestContainer, TypeA, TypeB>;
    using C1 = meta::InstantiateSortedT<TestContainer, TypeB, TypeA>;
    static_assert(std::is_same_v<C0, C1>);
}