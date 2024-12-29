#include <gtest/gtest.h>

#include "common/handle.hpp"
#include "scene/scene.hpp"

using namespace std::literals;

using namespace rn;

struct ComponentA
{
    int foo;
};

struct ComponentB
{
    float bar;
};

struct ComponentC
{
    std::string_view baz;
};

TEST(SceneTests, CanCreateEntity)
{
    scene::Scene scene;
    scene::Entity e = scene.AddEntity<ComponentA, ComponentB, ComponentC>({}, {}, {});
    ASSERT_TRUE(IsValid(e));
}

TEST(SceneTests, EntityGetsCreatedWithProvidedComponentContents)
{
    scene::Scene scene;
    scene::Entity e = scene.AddEntity<ComponentA, ComponentC>({ .foo = 128 }, { .baz = "Hello World!"sv });

    auto [a, c] = scene.Components<ComponentA, ComponentC>(e);
    ASSERT_EQ(a.foo, 128);
    ASSERT_EQ(c.baz, "Hello World!"sv);
}

TEST(SceneTests, CanRetrieveAndModifyComponentsInVaryingOrders)
{
    scene::Scene scene;
    scene::Entity e = scene.AddEntity<ComponentA, ComponentB, ComponentC>({}, {}, {});

    {
        auto [b, a] = scene.Components<ComponentB, ComponentA>(e);
        b.bar = 1.5f;
        a.foo = 20;
    }

    {
        auto [c] = scene.Components<ComponentC>(e);
        c.baz = "Hello World!"sv;
    }

    {
        auto [c, b, a] = scene.Components<ComponentC, ComponentB, ComponentA>(e);
        ASSERT_EQ(c.baz, "Hello World!"sv);
        ASSERT_FLOAT_EQ(b.bar, 1.5f);
        ASSERT_EQ(a.foo, 20);
    }
}

TEST(SceneTests, QueryReturnsAllCompatibleEntities)
{
    scene::Scene scene;
    scene::Entity e0 = scene.AddEntity<ComponentA, ComponentB>({}, {});
    scene::Entity e1 = scene.AddEntity<ComponentA, ComponentC>({}, {});
    scene::Entity e2 = scene.AddEntity<ComponentA>({});
    scene::Entity e3 = scene.AddEntity<ComponentB, ComponentC>({}, {});

    auto query0 = scene.Query<ComponentA>();
    auto query1 = scene.Query<ComponentA, ComponentC>();
    auto query2 = scene.Query<ComponentA, ComponentB>();

    for (auto [a] : query0)
    {
        a.foo = 500;
    }

    for (auto [a, c] : query1)
    {
        a.foo = 200;
    }

    for (auto [a, b] : query2)
    {
        a.foo = 100;
    }

    auto [a0] = scene.Components<ComponentA>(e0);
    auto [a1] = scene.Components<ComponentA>(e1);
    auto [a2] = scene.Components<ComponentA>(e2);
    ASSERT_EQ(a0.foo, 100);
    ASSERT_EQ(a1.foo, 200);
    ASSERT_EQ(a2.foo, 500);
}

TEST(SceneTests, QueriesCanHandleReadOnlyAccess)
{
    scene::Scene scene;
    scene::Entity e0 = scene.AddEntity<ComponentA, ComponentB>({ .foo = 100 }, {});
    scene::Entity e1 = scene.AddEntity<ComponentA>({ .foo = 100 });

    auto query = scene.Query<const ComponentA>();
    for (auto [a] : query)
    {
        ASSERT_EQ(a.foo, 100);
    }
}