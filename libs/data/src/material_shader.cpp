#include "data/material_shader.hpp"
#include "data/texture.hpp"

#include "asset/schema/reference_generated.h"
#include "data/schema/common_generated.h"
#include "data/schema/render_pass_generated.h"
#include "data/schema/material_shader_generated.h"

#include "common/log/log.hpp"

#include "rhi/device.hpp"
#include "rhi/format.hpp"

namespace rn::data
{
    namespace
    {
        static_assert(CountOf<MaterialRenderPass>() == CountOf<schema::render::MaterialRenderPass>());

        rhi::ShaderBytecode ToShaderBytecode(const schema::render::Bytecode* bytecode)
        {
            if (!bytecode)
            {
                return {};
            }

            return { bytecode->d3d12_pc()->data(), bytecode->d3d12_pc()->size() };
        }

        constexpr const MaterialShaderParameterType PARAMETER_TYPES[] = 
        {
            MaterialShaderParameterType::Count,
            MaterialShaderParameterType::Texture,
            MaterialShaderParameterType::FloatVec,
            MaterialShaderParameterType::UintVec,
            MaterialShaderParameterType::IntVec,
        };
        static_assert(RN_ARRAY_SIZE(PARAMETER_TYPES) == int(schema::render::ParameterType::MAX) + 1);

        MaterialShaderParameterType ToMaterialShaderParameterType(schema::render::ParameterType type)
        {
            return PARAMETER_TYPES[int(type)];
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
        void PopulateNumericLimits(NumericMaterialShaderParam<NumericType>& dest, const SchemaLimits* limits)
        {
            dest.dimension = std::min(limits->dimension(), uint32_t(dest.CAPACITY));
            if (limits->default_())
            {
                size_t count = std::min(dest.dimension, limits->default_()->size());
                std::memcpy(dest.defaultValue, limits->default_()->data(), count * sizeof(NumericType));
            }

            if (limits->min())
            {
                size_t count = std::min(dest.dimension, limits->min()->size());
                std::memcpy(dest.minValue, limits->min()->data(), count * sizeof(NumericType));
            }

            if (limits->max())
            {
                size_t count = std::min(dest.dimension, limits->max()->size());
                std::memcpy(dest.maxValue, limits->max()->data(), count * sizeof(NumericType));
            }
        }
    }

    MaterialShaderBuilder::MaterialShaderBuilder(rhi::Device* device)
        : _device(device)
    {}

    MaterialShaderData MaterialShaderBuilder::Build(const asset::AssetBuildDesc& desc)
    {
        using namespace schema;
        const render::MaterialShader* asset = render::GetMaterialShader(desc.data.data());

        size_t rasterPassCount = 0;
        rasterPassCount += asset->vertex_raster_passes() ? asset->vertex_raster_passes()->size() : 0;
        rasterPassCount += asset->mesh_raster_passes() ? asset->mesh_raster_passes()->size() : 0;

        size_t rtPassCount = asset->ray_tracing_passes() ? asset->ray_tracing_passes()->size() : 0;

        MaterialShaderRasterPass* rasterPasses = rasterPassCount ? TrackedNewArray<MaterialShaderRasterPass>(MemoryCategory::Data, rasterPassCount) : nullptr;
        MaterialShaderRTPass* rtPasses = rtPassCount ? TrackedNewArray<MaterialShaderRTPass>(MemoryCategory::Data, rtPassCount) : nullptr;

        uint32_t rasterPassIdx = 0;
        if (asset->vertex_raster_passes())
        {
            for (const render::VertexRasterPass* pass : *asset->vertex_raster_passes())
            {
                rhi::VertexRasterPipelineDesc pipelineDesc = {
                    .flags = rhi::RasterPipelineFlags::RequiresInputLayoutForDrawID,
                    .vertexShaderBytecode = ToShaderBytecode(pass->vertex_shader()),
                    .pixelShaderBytecode = ToShaderBytecode(pass->pixel_shader())
                };

                MaterialRenderPass materialPass = static_cast<MaterialRenderPass>(pass->render_pass());
                PopulateRenderStateForPass(materialPass, pipelineDesc);

                rasterPasses[rasterPassIdx++] = {
                    .type = RasterPassType::Vertex,
                    .renderPass = materialPass,
                    .pipeline = _device->CreateRasterPipeline(pipelineDesc)
                };
            }
        }

        if (asset->mesh_raster_passes())
        {
            for (const render::MeshRasterPass* pass : *asset->mesh_raster_passes())
            {
                rhi::MeshRasterPipelineDesc pipelineDesc = {
                    .amplificationShaderBytecode = ToShaderBytecode(pass->amp_shader()),
                    .meshShaderBytecode = ToShaderBytecode(pass->mesh_shader()),
                    .pixelShaderBytecode = ToShaderBytecode(pass->pixel_shader())
                };

                MaterialRenderPass materialPass = static_cast<MaterialRenderPass>(pass->render_pass());
                PopulateRenderStateForPass(materialPass, pipelineDesc);

                rasterPasses[rasterPassIdx++] = {
                    .type = RasterPassType::Mesh,
                    .renderPass = materialPass,
                    .pipeline = _device->CreateRasterPipeline(pipelineDesc)
                };
            }
        }

        if (asset->ray_tracing_passes())
        {
            uint32_t rtPassIdx = 0;
            for (const render::RayTracingPass* pass : *asset->ray_tracing_passes())
            {
                rtPasses[rtPassIdx++] = {
                    .renderPass = static_cast<MaterialRenderPass>(pass->render_pass()),
                    .closestHit = pass->closest_hit_export()->c_str(),
                    .anyHit = pass->any_hit_export()->c_str(),
                    .hitGroup = pass->hit_group_name()->c_str()
                };
            }
        }

        MaterialShaderParameter* parameters = nullptr;
        uint32_t parameterCount = 0;
        if (asset->parameter_groups())
        {
            for (const render::ParameterGroup* group : *asset->parameter_groups())
            {
                parameterCount += group->parameters()->size();
            }

            parameters = TrackedNewArray<MaterialShaderParameter>(MemoryCategory::Data, parameterCount);

            uint32_t paramIdx = 0;
            for (const render::ParameterGroup* group : *asset->parameter_groups())
            {
                for (const render::Parameter* p : *group->parameters())
                {
                    MaterialShaderParameter& param = parameters[paramIdx++];
                    param.type = ToMaterialShaderParameterType(p->data_type());
                    param.name = p->name()->c_str();
                    param.offsetInMaterialData = p->offset_in_buffer();
                    
                    switch (p->data_type())
                    {
                        case render::ParameterType::TextureParameter:
                        {
                            const render::TextureParameter* texture = p->data_as_TextureParameter();
                            param.pTexture = {
                                .dimension = TextureMaterialShaderDimension(texture->dimension()),
                                .defaultValue = texture->default_() ?
                                    Texture(desc.dependencies[texture->default_()->identifier()]) : 
                                    Texture::Invalid,
                            };
                        }
                        break;
                        case render::ParameterType::FloatVecParameter:
                        {
                            const render::FloatVecParameter* floatVec = p->data_as_FloatVecParameter();
                            PopulateNumericLimits(param.pFloatVec, floatVec);
                        }
                        break;
                        case render::ParameterType::UintVecParameter:
                        {
                            const render::UintVecParameter* uintVec = p->data_as_UintVecParameter();
                            PopulateNumericLimits(param.pUintVec, uintVec);
                        }
                        break;
                        case render::ParameterType::IntVecParameter:
                        {
                            const render::IntVecParameter* intVec = p->data_as_IntVecParameter();
                            PopulateNumericLimits(param.pIntVec, intVec);
                        }
                        break;
                    }
                }
            }
        }

        Span<uint8_t> rtLibrary = {};
        if (asset->ray_tracing_library())
        {
            uint32_t libSize = asset->ray_tracing_library()->d3d12_pc()->size();
            rtLibrary = { TrackedNewArray<uint8_t>(MemoryCategory::Data, libSize), libSize };
            std::memcpy(rtLibrary.data(), asset->ray_tracing_library()->d3d12_pc()->data(), libSize);
        }

        MaterialShaderData outData = {
            .rasterPasses = { rasterPasses, rasterPassCount },
            .rtPasses = {rtPasses, rtPassCount },
            .parameters = {parameters, parameterCount },
            .rtLibrary = rtLibrary
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