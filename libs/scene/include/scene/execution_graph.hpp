#pragma once

#include "common/common.hpp"
#include "common/memory/span.hpp"
#include "common/memory/hash_set.hpp"
#include "scene/scene.hpp"

#include "TaskScheduler.h"

namespace rn::scene
{
    enum class AccessType : uint32_t
    {
        Read = 0,
        Write,

        Count
    };

    struct ComponentAccessDesc
    {
        AccessType type;
        uint64_t componentID;
    };

    

    template <typename C>
    constexpr ComponentAccessDesc ToComponentAccessDesc() 
    { 
        return {
            .type = std::is_const_v<C> ? AccessType::Read : AccessType::Write,
            .componentID = TypeID<C>()
        };
    }

    template <typename... Args>
    struct TypeList{};

    template <typename E>
    struct Environment
    {
        using AccessType = E&;
        using Types = TypeList<E>;
    };

    template <typename... Cs>
    using SelectResult = std::tuple<Cs&...>;

    template <typename... Cs>
    struct Select
    {
        using QueryType = QueryResult<Cs...>;
        using AccessType = const SelectResult<Cs...>&;
        using Types = TypeList<Cs...>;

        static constexpr const ComponentAccessDesc Components[] = { ToComponentAccessDesc<Cs>()... };
        static QueryType Query(Scene& scene) { return scene.Query<Cs...>(); }
    };

    template <typename... Cs>
    struct Query
    {
        using QueryType = QueryResult<Cs...>;
        using AccessType = const QueryResult<Cs...>&;
        using Types = TypeList<Cs...>;

        static constexpr const ComponentAccessDesc Components [] = { ToComponentAccessDesc<Cs>()... };
        static QueryType Query(Scene& scene) { return scene.Query<Cs...>(); }
    };

    template <typename S, typename... Access>
    using FnExecuteSystem = void(*)(typename S::AccessType, typename Access::AccessType...);

    class ExecutionGraph
    {
    public:

        template <typename T, typename... Args>
        void EmplaceEnvironment(Args&&... args);

        template <typename S, typename... Access>
        void RegisterSystem(FnExecuteSystem<S, Access...> onExecute);

    private:
        using FnOpaque = void(*)(void);
        using FnSystem = void(*)(Scene&, FnOpaque);

        struct TypeAccessData
        {
            AccessType lastAccess = AccessType::Count;
            Vector<uint16_t> systemIndices = MakeVector<uint16_t>(MemoryCategory::Scene);
        };
        using TypeAccessMap = HashMap<uint64_t, TypeAccessData>;

        template <typename T>
        void PushTypeAccess(uint16_t sysIdx, ScopedHashSet<uint16_t>& dependencies);

        template <typename... T>
        void PushTypeListAccess(TypeList<T...> list, uint16_t sysIdx, ScopedHashSet<uint16_t>& dependencies);

        Vector<enki::ITaskSet*> _tasks = MakeVector<enki::ITaskSet*>(MemoryCategory::Scene);
        TypeAccessMap _typeAccesses = MakeHashMap<uint64_t, TypeAccessData>(MemoryCategory::Scene);
    };

    template <typename T>
    void ExecutionGraph::PushTypeAccess(uint16_t sysIdx, ScopedHashSet<uint16_t>& dependencies)
    {
        TypeAccessData& access = _typeAccesses[TypeID<T>()];
        if constexpr (std::is_const_v<T>)
        {
            if (access.lastAccess == AccessType::Read)
            {
                access.systemIndices.push_back(sysIdx);
            }
            else
            {
                for (uint16_t depIdx : access.systemIndices)
                {
                    dependencies.emplace(depIdx);
                }

                access.systemIndices = { sysIdx };
            }

            access.lastAccess = AccessType::Read;
        }
        else
        {
            for (uint16_t depIdx : access.systemIndices)
            {
                dependencies.emplace(depIdx);
            }
            access.systemIndices = { sysIdx };
            access.lastAccess = AccessType::Write;
        }
    }

    template <typename... T>
    void ExecutionGraph::PushTypeListAccess(TypeList<T...> list, uint16_t sysIdx, ScopedHashSet<uint16_t>& dependencies)
    {
        (PushTypeAccess<T>(sysIdx), ...);
    }

    template <typename S, typename... Access>
    void ExecutionGraph::RegisterSystem(FnExecuteSystem<S, Access...> system)
    {
        RN_ASSERT(_systems.size() < std::numeric_limits<uint16_t>::max());

        MemoryScope SCOPE;
        ScopedHashSet<uint16_t> dependencies;
        dependencies.reserve(32);

        uint16_t sysIdx = uint16_t(_systems.size());
        PushTypeListAccess(typename S::Types(), sysIdx, dependencies);
        (PushTypeListAccess(typename Access::Types(), sysIdx, dependencies), ...);

        auto ExecuteSystemTask = [system](enki::TaskSetPartition range, uint32_t threadnum)
        {
            const Scene* scene = nullptr;

            auto selectResult = S::Query(scene);
            for (auto it : selectResult)
            {
                
            }
        };

        

        enk::TaskSet* task = TrackedNew<enki::TaskSet>(MemoryCategory::Scene, ExecuteSystemTask);

        for (uint16_t depIdx : dependencies)
        {
            task->SetDependency()
        }
    }
}