#pragma once

#include "common/common.hpp"
#include "common/handle.hpp"

#include "common/memory/memory.hpp"
#include "common/memory/span.hpp"

#include "asset/asset.hpp"

namespace rn::data
{
    RN_MEMORY_CATEGORY(Data);

    RN_HANDLE(MaterialShader);
    RN_DEFINE_HANDLE(Material, 0x43)

    struct MaterialData
    {
        MaterialShader shader;
        Span<uint8_t> uniformData;
    };

    class MaterialBuilder : public asset::Builder<Material, MaterialData>
    {
        public:

        MaterialBuilder();

        MaterialData    Build(const asset::AssetBuildDesc& desc) override;
        void            Destroy(MaterialData& data) override;
        void            Finalize() override;
    };
}