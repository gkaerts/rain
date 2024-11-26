#pragma once

#include "common/common.hpp"
#include "common/handle.hpp"
#include "common/memory/vector.hpp"

#include "rhi/resource.hpp"
#include "rhi/command_list.hpp"
#include "rhi/transient_resource.hpp"

#include "asset/asset.hpp"

#include <mutex>
#include <thread>

namespace rn::rhi
{
    class Device;
    class CommandList;
}

namespace rn::data
{
    RN_MEMORY_CATEGORY(Data)
    RN_DEFINE_HANDLE(Texture, 0x40)

    enum class TextureType : uint32_t
    {
        Texture2D,
        Texture3D
    };

    struct TextureData
    {
        TextureType type;
        rhi::GPUMemoryRegion gpuRegion;
        rhi::TextureFormat format;
        union
        {
            struct 
            {
                uint32_t width;
                uint32_t height;
                uint32_t mipLevels;

                rhi::Texture2D rhiTexture;
                rhi::Texture2DView rhiView;
            } texture2D;

            struct 
            {
                uint32_t width;
                uint32_t height;
                uint32_t depth;
                uint32_t mipLevels;

                rhi::Texture3D rhiTexture;
                rhi::Texture3DView rhiView;
            } texture3D;
        };
    };

    class TextureAllocator
    {
    public:

        TextureAllocator(rhi::Device* device);

        rhi::GPUMemoryRegion Allocate(uint32_t sizeInBytes);
        void Free(rhi::GPUMemoryRegion& region);
        void PushBarrier(const rhi::Texture2DBarrier& barrier);
        void PushBarrier(const rhi::Texture3DBarrier& barrier);
        void FlushBarriers(rhi::CommandList* cl);

    private:

        rhi::TransientResourceAllocator _allocator;
        Vector<rhi::Texture2DBarrier> _2dBarriers = MakeVector<rhi::Texture2DBarrier>(MemoryCategory::Data);
        Vector<rhi::Texture3DBarrier> _3dBarriers = MakeVector<rhi::Texture3DBarrier>(MemoryCategory::Data);
        std::mutex _mutex;
    };

    class TextureBuilder : public asset::Builder<Texture, TextureData>
    {
    public:

        TextureBuilder(rhi::Device* device);

        TextureData Build(const asset::AssetBuildDesc& desc) override;
        void        Destroy(TextureData& data) override;
        void        Finalize() override;

    private:

        struct UploadCommandList
        {
            std::thread::id threadID;
            rhi::CommandList* cl;
        };

        rhi::CommandList* GetCommandListForCurrentThread();

        rhi::Device* _device;
        TextureAllocator _allocator;

        std::mutex _clMutex;
        Vector<UploadCommandList> _uploadCLs = MakeVector<UploadCommandList>(MemoryCategory::Data);
    };
}