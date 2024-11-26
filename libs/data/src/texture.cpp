#include "data/texture.hpp"
#include "data/schema/texture_generated.h"

#include "rhi/format.hpp"
#include "rhi/device.hpp"
#include "rhi/command_list.hpp"
#include "rhi/transient_resource.hpp"

#include "transcoder/basisu_transcoder.h"

#include <mutex>

namespace rn::data
{
    RN_MEMORY_CATEGORY(Data)

    namespace
    {
        const char* TEXTURE_EXTENSION = ".texture";
        const StringHash TEXTURE_EXTENSION_HASH = HashString(TEXTURE_EXTENSION);

        constexpr const basist::transcoder_texture_format TRANSCODER_FORMATS[] =
        {
            basist::transcoder_texture_format::cTFBC7_RGBA,             // Diffuse
            basist::transcoder_texture_format::cTFBC5_RG,               // Normal
            basist::transcoder_texture_format::cTFBC1_RGB,              // Control
            basist::transcoder_texture_format::cTFRGBA32,               // UI
            basist::transcoder_texture_format::cTFRGBA32,               // LUT
            basist::transcoder_texture_format::cTFBC4_R,                // Heightmap
        };
        RN_MATCH_ENUM_AND_ARRAY(TRANSCODER_FORMATS, schema::render::Usage);

        constexpr const rhi::TextureFormat TEXTURE_FORMATS[] = 
        {
            rhi::TextureFormat::BC7,        // Diffuse
            rhi::TextureFormat::BC5,        // Normal
            rhi::TextureFormat::BC1,        // Control
            rhi::TextureFormat::RGBA8Unorm, // UI
            rhi::TextureFormat::RGBA8Unorm, // LUT
            rhi::TextureFormat::BC4,        // Heightmap
        };
        RN_MATCH_ENUM_AND_ARRAY(TEXTURE_FORMATS, schema::render::Usage);  
    }

    TextureAllocator::TextureAllocator(rhi::Device* device)
        : _allocator(MemoryCategory::Data, device, 4096)
    {}

    rhi::GPUMemoryRegion TextureAllocator::Allocate(uint32_t sizeInBytes)
    {
        std::unique_lock lock(_mutex);
        return _allocator.AllocateMemoryRegion(sizeInBytes);
    }

    void TextureAllocator::Free(rhi::GPUMemoryRegion& region)
    {
        std::unique_lock lock(_mutex);
        _allocator.FreeMemoryRegion(region);
        region = {};
    }

    void TextureAllocator::PushBarrier(const rhi::Texture2DBarrier& barrier)
    {
        std::unique_lock lock(_mutex);
        _2dBarriers.push_back(barrier);
    }

    void TextureAllocator::PushBarrier(const rhi::Texture3DBarrier& barrier)
    {
        std::unique_lock lock(_mutex);
        _3dBarriers.push_back(barrier);
    }

    void TextureAllocator::FlushBarriers(rhi::CommandList* cl)
    {
        std::unique_lock lock(_mutex);
        cl->Barrier({
            .texture2DBarriers = _2dBarriers,
            .texture3DBarriers = _3dBarriers
        });

        _2dBarriers.clear();
        _3dBarriers.clear();
    }


    TextureBuilder::TextureBuilder(rhi::Device* device)
        : _device(device)
        , _allocator(device)
    {}

    namespace
    {
        TextureData TranscodeTexture2D(const char* name,
            rhi::Device* device,
            rhi::CommandList* cl,
            TextureAllocator& allocator,
            schema::render::Usage usage,
            basist::basisu_transcoder& transcoder,
            Span<const uint8_t> basisData)
        {
            basist::basisu_image_info imageInfo{};
            RN_ASSERT(transcoder.get_image_info(basisData.data(), uint32_t(basisData.size()), imageInfo, 0));

            const rhi::Texture2DDesc desc = {
                .flags = rhi::TextureCreationFlags::AllowShaderReadOnly,
                .width = imageInfo.m_width,
                .height = imageInfo.m_height,
                .arraySize = 1,
                .mipLevels = imageInfo.m_total_levels,
                .format = TEXTURE_FORMATS[int(usage)],
                .optClearValue = nullptr,
                .name = name
            };

            rhi::ResourceFootprint fp = device->CalculateTextureFootprint(desc);
            rhi::GPUMemoryRegion gpuRegion = allocator.Allocate(uint32_t(fp.sizeInBytes));

            rhi::Texture2D rhiTexture = device->CreateTexture2D(desc, gpuRegion);
            rhi::Texture2DView rhiView = device->CreateTexture2DView({
                .texture = rhiTexture,
                .format = desc.format,
                .mipCount = desc.mipLevels,
                .minLODClamp = 0.0f
            });

            uint32_t pixelOrBlockByteWidth = uint32_t(rhi::PixelOrBlockByteWidth(desc.format));
            uint32_t blockRowCount = rhi::BlockRowCount(desc.format);

            MemoryScope SCOPE;
            ScopedVector<rhi::MipUploadDesc> uploadDescs(desc.mipLevels);
            uint64_t totalBytes = device->CalculateMipUploadDescs(desc, uploadDescs);

            uint8_t* uploadData = static_cast<uint8_t*>(TrackedAlloc(MemoryCategory::Data, totalBytes, 16));

            size_t offsetInData = 0;
            for (uint32_t levelIdx = 0; levelIdx < desc.mipLevels; ++levelIdx)
            {
                const rhi::MipUploadDesc& mipDesc = uploadDescs[levelIdx];
                uint32_t mipSizeInBytes = uint32_t(mipDesc.totalSizeInBytes);
                RN_ASSERT(transcoder.transcode_image_level(
                        basisData.data(), 
                        uint32_t(basisData.size()),
                        0, 
                        levelIdx, 
                        uploadData + offsetInData,
                        mipSizeInBytes / pixelOrBlockByteWidth,
                        TRANSCODER_FORMATS[int(usage)],
                        0,
                        mipDesc.rowPitch / pixelOrBlockByteWidth,
                        nullptr,
                        mipDesc.rowCount * blockRowCount));

                offsetInData += mipSizeInBytes;
            }

            cl->Barrier({
                .texture2DBarriers = {
                    {
                        .fromStage = rhi::PipelineSyncStage::None,
                        .toStage = rhi::PipelineSyncStage::Copy,
                        .fromAccess = rhi::PipelineAccess::None,
                        .toAccess = rhi::PipelineAccess::CopyWrite,
                        .fromLayout = rhi::TextureLayout::Undefined,
                        .toLayout = rhi::TextureLayout::CopyWrite,
                        .handle = rhiTexture,
                        .firstMipLevel = 0,
                        .numMips = desc.mipLevels,
                        .firstArraySlice = 0,
                        .numArraySlices = 1
                    }
                }
            });

            cl->UploadTextureData(rhiTexture, 0, uploadDescs, { uploadData, totalBytes });
            allocator.PushBarrier({
                .fromStage = rhi::PipelineSyncStage::Copy,
                .toStage = rhi::PipelineSyncStage::VertexShader | rhi::PipelineSyncStage::PixelShader | rhi::PipelineSyncStage::ComputeShader,
                .fromAccess = rhi::PipelineAccess::CopyWrite,
                .toAccess = rhi::PipelineAccess::ShaderRead,
                .fromLayout = rhi::TextureLayout::CopyWrite,
                .toLayout = rhi::TextureLayout::ShaderRead,
                .handle = rhiTexture,
                .firstMipLevel = 0,
                .numMips = desc.mipLevels,
                .firstArraySlice = 0,
                .numArraySlices = 1
            });

            return
            {
                .type = TextureType::Texture2D,
                .gpuRegion = gpuRegion,
                .format = desc.format,
                .texture2D = {
                    .width = desc.width,
                    .height = desc.height,
                    .mipLevels = desc.mipLevels,

                    .rhiTexture = rhiTexture,
                    .rhiView = rhiView
                }
            };
        }
    }

    TextureData TextureBuilder::Build(const asset::AssetBuildDesc& desc)
    {
        const schema::render::Texture* asset = schema::render::GetTexture(desc.data.data());

        TextureData outData = {};

        RN_ASSERT(asset->data_format() == schema::render::TextureDataFormat::Basis);
        if(asset->data_format() == schema::render::TextureDataFormat::Basis)
        {
            Span<const uint8_t> basisData = { asset->data()->data(), asset->data()->size() };

            basist::basisu_transcoder transcoder;
            basist::basisu_file_info fileInfo{};
            transcoder.get_file_info(basisData.data(), uint32_t(basisData.size()), fileInfo);

            RN_ASSERT(transcoder.start_transcoding(basisData.data(), uint32_t(basisData.size())));
            switch (fileInfo.m_tex_type)
            {
            case basist::basis_texture_type::cBASISTexType2D:
                outData = TranscodeTexture2D(
                    desc.identifier,
                    _device,
                    GetCommandListForCurrentThread(),
                    _allocator,
                    asset->usage(),
                    transcoder,
                    basisData);
                break;

            case basist::basis_texture_type::cBASISTexTypeVolume:
            case basist::basis_texture_type::cBASISTexType2DArray:
            case basist::basis_texture_type::cBASISTexTypeCubemapArray:
            default:
                RN_ASSERT(false);
                break;
            }

            RN_ASSERT(transcoder.stop_transcoding());
        }
        else
        {

        }

        return outData;
    }

    void TextureBuilder::Destroy(TextureData& data)
    {
        switch(data.type)
        {
            case TextureType::Texture2D:
            {
                _device->Destroy(data.texture2D.rhiView);
                _device->Destroy(data.texture2D.rhiTexture);
            }
            break;

            case TextureType::Texture3D:
            {
                _device->Destroy(data.texture3D.rhiView);
                _device->Destroy(data.texture3D.rhiTexture);
            }
            break;

            default:
                RN_ASSERT(false);
            break;
        }

        _allocator.Free(data.gpuRegion);
        data = {};
    }

    void TextureBuilder::Finalize()
    {
        if (_uploadCLs.empty())
        {
            return;
        }

        rhi::CommandList* lastCL = _uploadCLs.back().cl;
        _allocator.FlushBarriers(lastCL);

        MemoryScope SCOPE;
        ScopedVector<rhi::CommandList*> submitCLs;
        for (const UploadCommandList& uploadCL : _uploadCLs)
        {
            submitCLs.push_back(uploadCL.cl);
        }

        _device->SubmitCommandLists(submitCLs);
    }

    rhi::CommandList* TextureBuilder::GetCommandListForCurrentThread()
    {
        rhi::CommandList* cl = nullptr;

        std::unique_lock lock(_clMutex);
        for (const UploadCommandList& uploadCL : _uploadCLs)
        {
            if (uploadCL.threadID == std::this_thread::get_id())
            {
                cl = uploadCL.cl;
                break;
            }
        }

        if (!cl)
        {
            cl = _device->AllocateCommandList();
            _uploadCLs.push_back({
                .threadID = std::this_thread::get_id(),
                .cl = cl
            });
        }

        return cl;
    }

}