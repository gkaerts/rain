#include <gtest/gtest.h>
#include "common/type_id.hpp"

struct DummyTypeA
{
    uint32_t foo;
    float bar;
};

struct DummyTypeB
{
    uint32_t bar;
    float foo;
};

template <uint64_t N>
struct ConstexprTest
{
    static constexpr const uint64_t value = N;
};

namespace testA
{
    struct TypeWithACommonName
    {
        float baz;
    };
}

namespace testB
{
    struct TypeWithACommonName
    {
        uint64_t baz;
    };
}

TEST(TypeIDTests, TypeIDsAreConstexpr)
{
    auto t = ConstexprTest<rn::TypeID<DummyTypeA>()>();
}

TEST(TypeIDTests, TypeIDsOfSameTypeAreEqual)
{
    ASSERT_EQ(rn::TypeID<DummyTypeA>(), rn::TypeID<DummyTypeA>());
    ASSERT_EQ(rn::TypeID<DummyTypeB>(), rn::TypeID<DummyTypeB>());
}

TEST(TypeIDTests, TypeIDsOfSameTypeWithDifferentCVRefAreEqual)
{
    ASSERT_EQ(rn::TypeID<DummyTypeA>(), rn::TypeID<const DummyTypeA>());
    ASSERT_EQ(rn::TypeID<DummyTypeA>(), rn::TypeID<DummyTypeA&>());
    ASSERT_EQ(rn::TypeID<DummyTypeA>(), rn::TypeID<const DummyTypeA&>());
    ASSERT_EQ(rn::TypeID<DummyTypeA>(), rn::TypeID<volatile DummyTypeA>());
    ASSERT_EQ(rn::TypeID<DummyTypeA>(), rn::TypeID<volatile DummyTypeA&>());

    ASSERT_NE(rn::TypeID<DummyTypeA>(), rn::TypeID<DummyTypeA*>());
    ASSERT_NE(rn::TypeID<DummyTypeA>(), rn::TypeID<const DummyTypeA*>());
    ASSERT_NE(rn::TypeID<DummyTypeA>(), rn::TypeID<volatile DummyTypeA*>());
}

TEST(TypeIDTests, TypeIDsAreUnique)
{
    ASSERT_NE(rn::TypeID<DummyTypeA>(), rn::TypeID<DummyTypeB>());
}

TEST(TypeIDTests, TypesWithSharedNamesGenerateDifferentTypeIDs)
{
    ASSERT_NE(rn::TypeID<testA::TypeWithACommonName>(), rn::TypeID<testB::TypeWithACommonName>());
}