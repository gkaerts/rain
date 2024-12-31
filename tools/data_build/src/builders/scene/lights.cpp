#pragma warning(push, 1)
#include <pxr/usd/usdLux/sphereLight.h>
#include <pxr/usd/usdLux/blackbody.h>
#include "rn/omniLightComponentAPI.h"
#pragma warning(pop)

#include "builders/build.hpp"
#include "builders/usd.hpp"
#include "builders/scene/scene.hpp"

#include "lights_gen.hpp"

namespace rn
{
    const PrimSchema OMNI_LIGHT_PRIM_SCHEMA = {
        .requiredProperties = {
            { .name = "omnilight:source", .fnIsValidRelationship = RelatedPrimIsA<pxr::UsdLuxSphereLight> },
            { .name = "omnilight:boundingRadius" }
        },
        .optionalProperties = {
            { .name = "omnilight:castShadow" }
        }
    };

    bool BuildOmniLightComponent(std::string_view file, const pxr::RnOmniLightComponentAPI& usdLight, BumpAllocator& blobAllocator, render::schema::OmniLight& outLight)
    {
        pxr::SdfPathVector sphereLightPaths = ResolveRelationTargets(usdLight.GetSourceRel());
        pxr::UsdLuxSphereLight sphereLight(usdLight.GetPrim().GetStage()->GetPrimAtPath(sphereLightPaths[0]));

        pxr::GfVec3f color = Value<pxr::GfVec3f>(sphereLight.GetColorAttr());
        bool useColorTemperature = Value<bool>(sphereLight.GetEnableColorTemperatureAttr());
        if (useColorTemperature)
        {
            color = pxr::GfCompMult(color, pxr::UsdLuxBlackbodyTemperatureAsRgb(Value<float>(sphereLight.GetColorTemperatureAttr())));
        }

        float exposure = powf(2.0f, Value<float>(sphereLight.GetExposureAttr()));

        outLight = {
            .color = { color[0], color[1], color[2] },
            .radius = Value<float>(usdLight.GetBoundingRadiusAttr()),
            .sourceRadius = Value<float>(sphereLight.GetRadiusAttr()),
            .intensity = exposure * Value<float>(sphereLight.GetIntensityAttr()),
            .castShadow = Value<bool>(usdLight.GetCastShadowAttr()),
        };
        return true;
    }

    struct RegisterLightComponents
    {
        RegisterLightComponents()
        {
            const ComponentHandler handlers[] = {
                MakeHasAPIHandler<pxr::RnOmniLightComponentAPI, render::schema::OmniLight, BuildOmniLightComponent>(),
            };

            for (const ComponentHandler& handler : handlers)
            {
                RegisterComponentHandler(handler);
            }
        }
    };

    static RegisterLightComponents _RegisterLights;
}