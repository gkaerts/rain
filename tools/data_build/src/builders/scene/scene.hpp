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
    using FnPrimHasRnAPI = bool(*)(const pxr::UsdPrim&);
    using FnBuildComponent = bool(*)(std::string_view, const pxr::UsdPrim&, BumpAllocator&, Vector<scene::schema::Component>&);

    struct ComponentHandler
    {
        FnPrimHasRnAPI fnHasAPI;
        FnBuildComponent fnBuildComponent;
        uint64_t typeID;
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

    template <typename UsdType, typename SchemaType, bool Op(std::string_view, const UsdType&, BumpAllocator&, SchemaType&)>
    ComponentHandler MakeIsAHandler()
    {
        return {
            .fnHasAPI = [](const pxr::UsdPrim& prim) { return prim.IsA<UsdType>(); },
            .fnBuildComponent = [](std::string_view file, const pxr::UsdPrim& prim, BumpAllocator& allocator, Vector<scene::schema::Component>& outComponents)
            {
                SchemaType component = {};
                if (!Op(file, UsdType(prim), allocator, component))
                {
                    return false;
                }

                PushComponent(component, allocator, outComponents);
                return true;
            },
            .typeID = TypeID<SchemaType>()
        };
    }

    template <typename UsdType, typename SchemaType, bool Op(std::string_view, const UsdType&, BumpAllocator&, SchemaType&)>
    ComponentHandler MakeHasAPIHandler()
    {
        return {
            .fnHasAPI = [](const pxr::UsdPrim& prim) { return prim.HasAPI<UsdType>(); },
            .fnBuildComponent = [](std::string_view file, const pxr::UsdPrim& prim, BumpAllocator& allocator, Vector<scene::schema::Component>& outComponents)
            {
                SchemaType component = {};
                if (!Op(file, UsdType(prim), allocator, component))
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
    int ProcessUsdScene(std::string_view file, const DataBuildOptions& options, const pxr::UsdPrim& prim, Vector<std::string>& outFiles);
}