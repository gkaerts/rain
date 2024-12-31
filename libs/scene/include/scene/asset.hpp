#pragma once

#include "common/common.hpp"
#include "common/handle.hpp"
#include "common/memory/hash_map.hpp"

#include "asset/asset.hpp"
#include "scene/scene.hpp"

namespace rn::scene
{
    class SceneBuilder : public asset::Builder<Scene>
    {
    public:

        SceneBuilder();

        Scene   Build(const asset::AssetBuildDesc& desc) override;
        void    Destroy(Scene& scene) override;
        void    Finalize() override;
    };
}