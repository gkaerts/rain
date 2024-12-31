#pragma once

#include "common/common.hpp"
#include "common/handle.hpp"
#include "common/memory/memory.hpp"
#include "common/memory/span.hpp"
#include "common/math/aabb.hpp"

#include "asset/asset.hpp"

#include "rhi/resource.hpp"
#include "rhi/command_list.hpp"
#include "rhi/transient_resource.hpp"

namespace rn::rhi
{
    class Device;
    class CommandList;
}

namespace rn::data
{
    RN_MEMORY_CATEGORY(Data)

    enum class VertexStreamFormat : uint32_t
    {
        Float = 0,
        Half,

        Int,
        Uint,
        Int16,
        Uint16,
        Int8,
        Uint8,

        Count
    };

    struct VertexStream
    {
        rhi::BufferView     view;
        VertexStreamFormat  format;
        uint32_t            componentCount;
        uint32_t            elementStride;
        uint32_t            offsetInDataBuffer;
    };

    struct GeometryPart
    {
        uint32_t            materialIndex;
        uint32_t            baseVertex;

        rhi::BufferView     indices;
        rhi::IndexFormat    indexFormat;
        uint32_t            indexOffset;
        uint32_t            indexCount;
    };

    struct GeometryData
    {
        rhi::GPUMemoryRegion        dataGPURegion;
        rhi::Buffer                 dataBuffer;

        rhi::GPUMemoryRegion        blasGPURegion;
        rhi::Buffer                 blasBuffer;
        rhi::BLASView               blas;

        uint32_t                    vertexCount;

        VertexStream                positions;
        VertexStream                normals;
        VertexStream                tangents;
        VertexStream                colors;
        VertexStream                uvs;

        Span<const GeometryPart>    parts;
        AABB                        aabb;
    };

    class GeometryAllocator
    {
    public:
        GeometryAllocator(rhi::Device* device);

        rhi::GPUMemoryRegion Allocate(uint32_t sizeInBytes);
        void Free(rhi::GPUMemoryRegion& region);
        void PushBarrier(const rhi::BufferBarrier& barrier);
        void FlushBarriers(rhi::CommandList* cl);
    
    private:

        rhi::TransientResourceAllocator _allocator;
        Vector<rhi::BufferBarrier> _barriers = MakeVector<rhi::BufferBarrier>(MemoryCategory::Data);
        std::mutex _mutex;
    };

    class GeometryBuilder : public asset::Builder<GeometryData>
    {
    public:

        GeometryBuilder(rhi::Device* device);

        GeometryData    Build(const asset::AssetBuildDesc& desc) override;
        void            Destroy(GeometryData& data) override;
        void            Finalize() override;

    private:

        struct UploadCommandList
        {
            std::thread::id threadID;
            rhi::CommandList* cl;
        };

        rhi::CommandList* GetCommandListForCurrentThread();

        rhi::Device* _device;
        GeometryAllocator _allocator;

        std::mutex _clMutex;
        Vector<UploadCommandList> _uploadCLs = MakeVector<UploadCommandList>(MemoryCategory::Data);
    };
}