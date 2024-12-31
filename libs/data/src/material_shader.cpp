#include "data/material_shader.hpp"
#include "data/texture.hpp"

#include "material_shader_gen.hpp"
#include "luagen/schema.hpp"

#include "common/log/log.hpp"

#include "rhi/device.hpp"
#include "rhi/format.hpp"

namespace rn::data
{
    namespace
    {
        rhi::ShaderBytecode ToShaderBytecode(const schema::Bytecode& bytecode)
        {
            return bytecode.d3d12;
        }

        void PopulateRenderStateForPass(MaterialRenderPass pass,  rhi::VertexRasterPipelineDesc& desc)
        {
            // You don't have any actual rendering code yet :(
            static const rhi::RenderTargetFormat DUMMY_FORMATS[] = {
                rhi::RenderTargetFormat::RGBA8Unorm
            };

            RN_REMINDME("You need to actually provide non-dummy data in material_shader.cpp:PopulateRenderStateForPass()");
            desc.rasterizerState = rhi::RasterizerState::SolidBackFaceCull;
            desc.blendState = rhi::BlendState::Disabled;
            desc.depthState = rhi::DepthState::DepthReadGreaterEqual;
            desc.depthFormat = rhi::DepthFormat::D32Float;
            desc.renderTargetFormats = DUMMY_FORMATS;
        }

        void PopulateRenderStateForPass(MaterialRenderPass pass,  rhi::MeshRasterPipelineDesc& desc)
        {
            // You don't have any actual rendering code yet :(
            static const rhi::RenderTargetFormat DUMMY_FORMATS[] = {
                rhi::RenderTargetFormat::RGBA8Unorm
            };

            RN_REMINDME("You need to actually provide non-dummy data in material_shader.cpp:PopulateRenderStateForPass()");
            desc.rasterizerState = rhi::RasterizerState::SolidBackFaceCull;
            desc.blendState = rhi::BlendState::Disabled;
            desc.depthState = rhi::DepthState::DepthReadGreaterEqual;
            desc.depthFormat = rhi::DepthFormat::D32Float;
            desc.renderTargetFormats = DUMMY_FORMATS;
        }

        template <typename NumericType, typename SchemaLimits>
        void PopulateNumericLimits(NumericMaterialShaderParam<NumericType>& dest, const SchemaLimits& limits)
        {
            dest.dimension = std::min(limits.dimension, uint32_t(dest.CAPACITY));

            size_t count = std::min(size_t(dest.dimension), limits.defaultValue.size());
            std::memcpy(dest.defaultValue, limits.defaultValue.data(), count * sizeof(NumericType));

            count = std::min(size_t(dest.dimension), limits.minValue.size());
            std::memcpy(dest.minValue, limits.minValue.data(), count * sizeof(NumericType));

            count = std::min(size_t(dest.dimension), limits.maxValue.size());
            std::memcpy(dest.maxValue, limits.maxValue.data(), count * sizeof(NumericType));
        }
    }

    MaterialShaderBuilder::MaterialShaderBuilder(rhi::Device* device)
        : _device(device)
    {}

    MaterialShaderData MaterialShaderBuilder::Build(const asset::AssetBuildDesc& desc)
    {
        using namespace schema;
        MemoryScope SCOPE;
        schema::MaterialShader asset = rn::Deserialize<schema::MaterialShader>(desc.data, [](size_t size) { return ScopedAlloc(size, CACHE_LINE_TARGET_SIZE); });

        size_t rasterPassCount = 0;
        rasterPassCount += asset.vertexRasterPasses.size();
        rasterPassCount += asset.meshRasterPasses.size();

        size_t rtPassCount = asset.rayTracingPasses.size();

        MaterialShaderRasterPass* rasterPasses = rasterPassCount ? TrackedNewArray<MaterialShaderRasterPass>(MemoryCategory::Data, rasterPassCount) : nullptr;
        MaterialShaderRTPass* rtPasses = rtPassCount ? TrackedNewArray<MaterialShaderRTPass>(MemoryCategory::Data, rtPassCount) : nullptr;

        uint32_t rasterPassIdx = 0;
        for (const schema::VertexRasterPass& pass : asset.vertexRasterPasses)
        {
            rhi::VertexRasterPipelineDesc pipelineDesc = {
                .flags = rhi::RasterPipelineFlags::RequiresInputLayoutForDrawID,
                .vertexShaderBytecode = pass.vertexShader.d3d12,
                .pixelShaderBytecode = pass.pixelShader.d3d12
            };

            PopulateRenderStateForPass(pass.renderPass, pipelineDesc);

            rasterPasses[rasterPassIdx++] = {
                .type = RasterPassType::Vertex,
                .renderPass = pass.renderPass,
                .pipeline = _device->CreateRasterPipeline(pipelineDesc)
            };
        }
        
        for (const schema::MeshRasterPass& pass : asset.meshRasterPasses)
        {
            rhi::MeshRasterPipelineDesc pipelineDesc = {
                .amplificationShaderBytecode = pass.ampShader.d3d12,
                .meshShaderBytecode = pass.meshShader.d3d12,
                .pixelShaderBytecode = pass.pixelShader.d3d12
            };

            PopulateRenderStateForPass(pass.renderPass, pipelineDesc);

            rasterPasses[rasterPassIdx++] = {
                .type = RasterPassType::Mesh,
                .renderPass = pass.renderPass,
                .pipeline = _device->CreateRasterPipeline(pipelineDesc)
            };
        }
        
        uint32_t rtPassIdx = 0;
        for (const schema::RayTracingPass& pass : asset.rayTracingPasses)
        {
            rtPasses[rtPassIdx++] = {
                .renderPass = pass.renderPass,
                .closestHit = { pass.closestHitExport.data(), pass.closestHitExport.size() },
                .anyHit = { pass.anyHitExport.data(), pass.anyHitExport.size() },
                .hitGroup = { pass.hitGroupName.data(), pass.hitGroupName.size() }
            };
        }
        

        size_t parameterCount = 0;
        for (const schema::ParameterGroup& group : asset.parameterGroups)
        {
            parameterCount += group.textureParameters.size();
            parameterCount += group.floatVecParameters.size();
            parameterCount += group.uintVecParameters.size();
            parameterCount += group.intVecParameters.size();
        }

        MaterialShaderParameter* parameters = TrackedNewArray<MaterialShaderParameter>(MemoryCategory::Data, parameterCount);

        uint32_t paramIdx = 0;
        for (const schema::ParameterGroup& group : asset.parameterGroups)
        {
            for (const schema::TextureParameter& p : group.textureParameters)
            {
                MaterialShaderParameter& param = parameters[paramIdx++];
                param.type = MaterialShaderParameterType::Texture;
                param.name = { p.name.data(), p.name.size() };
                param.offsetInMaterialData = p.offsetInBuffer;
                param.pTexture = {
                    .type = p.type,
                    .defaultValue = desc.dependencies[p.defaultValue.identifier]
                };
            }

            for (const schema::FloatVecParameter& p : group.floatVecParameters)
            {
                MaterialShaderParameter& param = parameters[paramIdx++];
                param.type = MaterialShaderParameterType::FloatVec;
                param.name = { p.name.data(), p.name.size() };
                param.offsetInMaterialData = p.offsetInBuffer;
                PopulateNumericLimits(param.pFloatVec, p);
            }

            for (const schema::UintVecParameter& p : group.uintVecParameters)
            {
                MaterialShaderParameter& param = parameters[paramIdx++];
                param.type = MaterialShaderParameterType::UintVec;
                param.name = { p.name.data(), p.name.size() };
                param.offsetInMaterialData = p.offsetInBuffer;
                PopulateNumericLimits(param.pFloatVec, p);
            }

            for (const schema::IntVecParameter& p : group.intVecParameters)
            {
                MaterialShaderParameter& param = parameters[paramIdx++];
                param.type = MaterialShaderParameterType::IntVec;
                param.name = { p.name.data(), p.name.size() };
                param.offsetInMaterialData = p.offsetInBuffer;
                PopulateNumericLimits(param.pFloatVec, p);
            }
        }
        

        Span<uint8_t> rtLibrary = {};
        if (asset.rayTracingLibrary.d3d12.size())
        {
            size_t libSize = asset.rayTracingLibrary.d3d12.size();
            rtLibrary = { TrackedNewArray<uint8_t>(MemoryCategory::Data, libSize), libSize };
            std::memcpy(rtLibrary.data(), asset.rayTracingLibrary.d3d12.data(), libSize);
        }

        MaterialShaderData outData = {
            .rasterPasses = { rasterPasses, rasterPassCount },
            .rtPasses = {rtPasses, rtPassCount },
            .parameters = {parameters, parameterCount },
            .rtLibrary = rtLibrary,
            .uniformBufferSize = asset.uniformDataSize
        };

        return outData;
    }

    void MaterialShaderBuilder::Destroy(MaterialShaderData& data)
    {
        for (const MaterialShaderRasterPass& pass : data.rasterPasses)
        {
            _device->Destroy(pass.pipeline);
        }

        if (!data.rasterPasses.empty())
        {
            TrackedDeleteArray(data.rasterPasses.data());
        }

        if (!data.rtPasses.empty())
        {
            TrackedDeleteArray(data.rtPasses.data());
        }

        if (!data.parameters.empty())
        {
            TrackedDeleteArray(data.parameters.data());
        }

        if (!data.rtLibrary.empty())
        {
            TrackedDeleteArray(data.rtLibrary.data());
        }
    }

    void MaterialShaderBuilder::Finalize()
    {
        
    }

}