#pragma once

#pragma warning(push, 1)
#include <pxr/usd/usd/prim.h>
#pragma warning(pop)

#include "scene_gen.hpp"

#include "common/type_id.hpp"
#include "common/memory/bump_allocator.hpp"
#include "common/memory/vector.hpp"
#include <string_view>

namespace rn
{
    using FnPrimHasRnAPI    = bool(*)(const pxr::UsdPrim&);
    using FnBuildComponent  = bool(*)(const DataBuildContext&, const pxr::UsdPrim&, BumpAllocator&, Vector<std::string>&, Vector<scene::schema::Component>&);

    struct ComponentHandler
    {
        FnPrimHasRnAPI      fnHasAPI;
        FnBuildComponent    fnBuildComponent;
        uint64_t            typeID;
    };

    template <typename T>
    void PushComponent(const T& data, BumpAllocator& blobAllocator, Vector<scene::schema::Component>& outComponents)
    {
        uint8_t* destPtr = static_cast<uint8_t*>(blobAllocator.Allocate(sizeof(T), alignof(T)));
        std::memcpy(destPtr, &data, sizeof(T));

        outComponents.push_back( {
            .typeID = TypeID<T>(),
            .data = { destPtr, sizeof(T) }
        });
    }

    template <typename F>
    struct FuncTraits;

    template <typename R, typename... Args>
    struct FuncTraits<R(*)(Args...)>
    {
        using ArgTuple = std::tuple<Args...>;

        template <size_t N>
        using ArgN = std::tuple_element_t<N, ArgTuple>;
    };

    template <auto Op>
    ComponentHandler MakeIsAHandler()
    {
        using UsdType =     std::remove_cvref_t<std::tuple_element_t<1, typename FuncTraits<decltype(&Op)>::ArgTuple>>;
        using SchemaType =  std::remove_cvref_t<std::tuple_element_t<4, typename FuncTraits<decltype(&Op)>::ArgTuple>>;

        return {
            .fnHasAPI = [](const pxr::UsdPrim& prim) { return prim.IsA<UsdType>(); },
            .fnBuildComponent = [](const DataBuildContext& ctxt, const pxr::UsdPrim& prim, BumpAllocator& allocator, Vector<std::string>& outReferences, Vector<scene::schema::Component>& outComponents)
            {
                SchemaType component = {};
                if (!Op(ctxt, UsdType(prim), allocator, outReferences, component))
                {
                    return false;
                }

                PushComponent(component, allocator, outComponents);
                return true;
            },
            .typeID = TypeID<SchemaType>()
        };
    }

    template <auto Op>
    ComponentHandler MakeHasAPIHandler()
    {
        using UsdType =     std::remove_cvref_t<std::tuple_element_t<1, typename FuncTraits<decltype(&Op)>::ArgTuple>>;
        using SchemaType =  std::remove_cvref_t<std::tuple_element_t<4, typename FuncTraits<decltype(&Op)>::ArgTuple>>;

        return {
            .fnHasAPI = [](const pxr::UsdPrim& prim) { return prim.HasAPI<UsdType>(); },
            .fnBuildComponent = [](const DataBuildContext& ctxt, const pxr::UsdPrim& prim, BumpAllocator& allocator, Vector<std::string>& outReferences, Vector<scene::schema::Component>& outComponents)
            {
                SchemaType component = {};
                if (!Op(ctxt, UsdType(prim), allocator, outReferences, component))
                {
                    return false;
                }

                PushComponent(component, allocator, outComponents);
                return true;
            },
            .typeID = TypeID<SchemaType>()
        };
        
    }

    void RegisterComponentHandler(const ComponentHandler& handler);

    struct DataBuildOptions;
    int ProcessUsdScene(const DataBuildContext& ctxt, const pxr::UsdPrim& prim, Vector<std::string>& outFiles);
}