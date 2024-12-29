#include "scene/execution_graph.hpp"

namespace rn::scene
{
    struct TestEnvironment{};
    struct ComponentA{};
    struct ComponentB{};
    struct ComponentC{};
    struct ComponentD{};

    void TestSystem(const SelectResult<ComponentA, ComponentB>& entity,
        const QueryResult<const ComponentC>& queryC,
        const QueryResult<const ComponentD>& queryD,
        TestEnvironment& env)
    {
        
    }

    void TestExecutionGraph()
    {
        Scene scene;


        ExecutionGraph graph;
        graph.RegisterSystem<
            Select<ComponentA, ComponentB>,
            Query<const ComponentC>,
            Query<const ComponentD>,
            Environment<TestEnvironment>
        >(TestSystem);

    }
}