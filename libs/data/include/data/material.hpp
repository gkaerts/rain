#pragma once

#include "common/common.hpp"
#include "common/handle.hpp"

#include "common/memory/memory.hpp"
#include "common/memory/span.hpp"

#include "asset/asset.hpp"

namespace rn::data
{
    RN_MEMORY_CATEGORY(Data);

    struct MaterialData
    {
        asset::AssetIdentifier  shader;
        Span<uint8_t>           uniformData;
    };

    class MaterialBuilder : public asset::Builder<MaterialData>
    {
        public:

        MaterialBuilder();

        MaterialData    Build(const asset::AssetBuildDesc& desc) override;
        void            Destroy(MaterialData& data) override;
        void            Finalize() override;
    };
}