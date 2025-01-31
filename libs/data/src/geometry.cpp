#include "data/geometry.hpp"

#include "common_gen.hpp"
#include "geometry_gen.hpp"

#include "rhi/device.hpp"
#include "luagen/schema.hpp"

namespace rn::data
{
    GeometryAllocator::GeometryAllocator(rhi::Device* device)
        : _allocator(MemoryCategory::Data, device, 4096)
    {}

    rhi::GPUMemoryRegion GeometryAllocator::Allocate(uint32_t sizeInBytes)
    {
        std::unique_lock lock(_mutex);
        return _allocator.AllocateMemoryRegion(sizeInBytes);
    }

    void GeometryAllocator::Free(rhi::GPUMemoryRegion& region)
    {
        std::unique_lock lock(_mutex);
        _allocator.FreeMemoryRegion(region);
        region = {};
    }

    void GeometryAllocator::PushBarrier(const rhi::BufferBarrier& barrier)
    {
        std::unique_lock lock(_mutex);
        _barriers.push_back(barrier);
    }

    void GeometryAllocator::FlushBarriers(rhi::CommandList* cl)
    {
        std::unique_lock lock(_mutex);
        cl->Barrier({
            .bufferBarriers = _barriers,
        });

        _barriers.clear();
    }


    GeometryBuilder::GeometryBuilder(rhi::Device* device)
        : _device(device)
        , _allocator(device)
    {}

    namespace
    {
        constexpr const size_t GEOMETRY_REGION_ALIGNMENT = 64;

        VertexStream MakeVertexStream(rhi::Device* device, rhi::Buffer dataBuffer, const schema::VertexStream& inStream)
        {
            if (!inStream.componentCount)
            {
                return {};
            }

            return {
                .view = device->CreateBufferView({
                        .buffer = dataBuffer,
                        .offsetInBytes = inStream.region.offsetInBytes,
                        .sizeInBytes = inStream.region.sizeInBytes
                    }),
                .format = inStream.format,
                .componentCount = inStream.componentCount,
                .elementStride = inStream.stride,
                .offsetInDataBuffer = inStream.region.offsetInBytes
            };
        }

        uint32_t CalculateIndexCount(rhi::IndexFormat indexFormat, size_t bufferSize)
        {
            constexpr const size_t INDEX_SIZES[CountOf<rhi::IndexFormat>()] = {
                sizeof(uint16_t),
                sizeof(uint32_t)
            };

            return uint32_t(bufferSize / INDEX_SIZES[int(indexFormat)]);
        }

        void BuildBLAS(GeometryData& geometry,
            rhi::Device* device, 
            rhi::CommandList* uploadCL, 
            GeometryAllocator* geometryAllocator, 
            rhi::Buffer dataBuffer,
            const std::string_view identifier)
        {
            MemoryScope SCOPE;
            ScopedVector<rhi::BLASTriangleGeometryDesc> blasGeometryDescs;
            blasGeometryDescs.reserve(geometry.parts.size());

            for (const GeometryPart& part : geometry.parts)
            {
                blasGeometryDescs.push_back({
                    .indexBuffer            = dataBuffer,
                    .offsetInIndexBuffer    = part.indexOffset,
                    .indexCount             = part.indexCount,
                    .indexFormat            = part.indexFormat,
                    .vertexBuffer           = dataBuffer,
                    .offsetInVertexBuffer   = geometry.positions.offsetInDataBuffer,
                    .vertexCount            = geometry.vertexCount,
                    .vertexStride           = geometry.positions.elementStride,
                    .positionFormat         = rhi::PositionFormat::XYZFloat
                });
            }

            rhi::ASFootprint fp = device->CalculateBLASFootprint(blasGeometryDescs);
            rhi::GPUMemoryRegion blasRegion = geometryAllocator->Allocate(uint32_t(fp.destDataSizeInBytes));
            rhi::Buffer blasBuffer = device->CreateBuffer({
                .flags  = rhi::BufferCreationFlags::AllowAccelerationStructure | 
                            rhi::BufferCreationFlags::AllowShaderReadWrite,
                .size   = uint32_t(fp.destDataSizeInBytes),
                .name   = identifier
            }, blasRegion);

            rhi::BLASView blasView = device->CreateBLASView({
                .asBuffer       = blasBuffer,
                .offsetInBytes  = uint32_t(blasRegion.offsetInAllocation),
                .sizeInBytes    = uint32_t(blasRegion.regionSize),
            });

            rhi::GPUMemoryRegion scratchRegion = geometryAllocator->Allocate(uint32_t(fp.scratchDataSizeInBytes));
            rhi::Buffer scratchBuffer = device->CreateBuffer({
                .flags  = rhi::BufferCreationFlags::AllowShaderReadWrite,
                .size   = uint32_t(fp.scratchDataSizeInBytes),
                .name   = "BLAS Scratch Buffer"
            }, scratchRegion);

            uploadCL->Barrier({
                .bufferBarriers = {{
                    .fromStage  = rhi::PipelineSyncStage::None,
                    .toStage    = rhi::PipelineSyncStage::BuildAccelerationStructure,
                    .fromAccess = rhi::PipelineAccess::None,
                    .toAccess   = rhi::PipelineAccess::ShaderReadWrite,

                    .handle     = scratchBuffer,
                    .offset     = 0,
                    .size       = uint32_t(fp.scratchDataSizeInBytes)
            }}});

            uploadCL->BuildBLAS({
                .blas                   = blasView,
                .blasBuffer             = blasBuffer,
                .offsetInBlasBuffer     = 0,
                .scratchBuffer          = scratchBuffer,
                .offsetInScratchBuffer  = 0,
                .geometry               = blasGeometryDescs
            });

            geometryAllocator->PushBarrier({
                .fromStage  = rhi::PipelineSyncStage::BuildAccelerationStructure,
                .toStage    = rhi::PipelineSyncStage::ComputeShader | 
                                rhi::PipelineSyncStage::PixelShader | 
                                rhi::PipelineSyncStage::RayTracing,
                .fromAccess = rhi::PipelineAccess::AccelerationStructureWrite,
                .toAccess   = rhi::PipelineAccess::AccelerationStructureRead | 
                                rhi::PipelineAccess::ShaderRead
            });

            device->Destroy(scratchBuffer);

            geometry.blasGPURegion  = blasRegion;
            geometry.blasBuffer     = blasBuffer;
            geometry.blas           = blasView;
        }
    }

    GeometryData GeometryBuilder::Build(const asset::AssetBuildDesc& desc)
    {
        schema::Geometry asset = rn::Deserialize<schema::Geometry>(desc.data, [](size_t size) { return ScopedAlloc(size, CACHE_LINE_TARGET_SIZE); });

        rhi::GPUMemoryRegion dataRegion = _allocator.Allocate(uint32_t(asset.data.size()));
        uint32_t dataBufferSize = uint32_t(uint32_t(asset.data.size()));
        rhi::Buffer dataBuffer = _device->CreateBuffer({
            .flags = rhi::BufferCreationFlags::AllowShaderReadOnly | rhi::BufferCreationFlags::AllowShaderReadWrite,
            .size = dataBufferSize,
            .name = desc.identifier
        }, dataRegion);

        const schema::VertexStream& positions = asset.positions;
        RN_ASSERT(
            positions.region.offsetInBytes % GEOMETRY_REGION_ALIGNMENT == 0 &&
            positions.type == schema::VertexStreamType::Position &&
            positions.format == VertexStreamFormat::Float &&
            positions.componentCount >= 3);

        GeometryData outData = {
            .dataGPURegion      = dataRegion,
            .dataBuffer         = dataBuffer,
            .vertexCount        = asset.positions.region.sizeInBytes / asset.positions.stride,
            .positions          = MakeVertexStream(_device, dataBuffer, asset.positions),
             .aabb = {
                .min = { asset.aabb.min.x, asset.aabb.min.y, asset.aabb.min.z },
                .max = { asset.aabb.max.x, asset.aabb.max.y, asset.aabb.max.z },
            }
        };

        for (const schema::VertexStream& stream : asset.vertexStreams)
        {
            VertexStream* destStream = nullptr;
            switch(stream.type)
            {
                case schema::VertexStreamType::Normal:
                    RN_ASSERT(outData.normals.view == rhi::BufferView::Invalid);
                    destStream = &outData.normals;
                break;
                case schema::VertexStreamType::Tangent:
                    RN_ASSERT(outData.tangents.view == rhi::BufferView::Invalid);
                    destStream = &outData.tangents;
                break;
                case schema::VertexStreamType::UV:
                    RN_ASSERT(outData.uvs.view == rhi::BufferView::Invalid);
                    destStream = &outData.uvs;
                break;
                case schema::VertexStreamType::Color:
                    RN_ASSERT(outData.colors.view == rhi::BufferView::Invalid);
                    destStream = &outData.colors;
                break;
            }

            if (destStream)
            {
                *destStream = MakeVertexStream(_device, dataBuffer, stream);
            }
        }

        Span<GeometryPart> outParts = { TrackedNewArray<GeometryPart>(MemoryCategory::Data, asset.parts.size()), asset.parts.size() };

        uint32_t partIdx = 0;
        for (const schema::GeometryPart& part : asset.parts)
        {
            RN_ASSERT(part.indices.sizeInBytes > 0 &&
                part.indices.offsetInBytes % GEOMETRY_REGION_ALIGNMENT == 0);

            GeometryPart& outPart = outParts[partIdx++];
            outPart = 
            {
                .materialIndex  = part.materialIdx,
                .baseVertex     = part.baseVertex,
                .indices        = _device->CreateBufferView({
                                    .buffer = dataBuffer,
                                    .offsetInBytes = part.indices.offsetInBytes,
                                    .sizeInBytes = part.indices.sizeInBytes
                                }),

                .indexFormat    = part.indexFormat,
                .indexOffset    = part.indices.offsetInBytes,
                .indexCount     = CalculateIndexCount(part.indexFormat, part.indices.sizeInBytes)
            };
        }
        outData.parts = outParts;
        

        rhi::CommandList* uploadCL = GetCommandListForCurrentThread();
        rhi::TemporaryResource tempData = uploadCL->AllocateTemporaryResource(uint32_t(asset.data.size()));
        std::memcpy(tempData.cpuPtr, asset.data.data(), asset.data.size());

        uploadCL->Barrier({
            .bufferBarriers = {{
                .fromStage  = rhi::PipelineSyncStage::None,
                .toStage    = rhi::PipelineSyncStage::Copy,
                .fromAccess = rhi::PipelineAccess::None,
                .toAccess   = rhi::PipelineAccess::CopyWrite,
                .handle     = dataBuffer,
                .offset     = 0,
                .size       = dataBufferSize
            }}
        });

        uploadCL->CopyBufferRegion(dataBuffer, 0, tempData.buffer, tempData.offsetInBytes, dataBufferSize);

        _allocator.PushBarrier({
            .fromStage  = rhi::PipelineSyncStage::Copy,
            .toStage    = rhi::PipelineSyncStage::VertexShader | 
                            rhi::PipelineSyncStage::PixelShader | 
                            rhi::PipelineSyncStage::ComputeShader,
            .fromAccess = rhi::PipelineAccess::CopyWrite,
            .toAccess   = rhi::PipelineAccess::ShaderRead,
            .handle     = dataBuffer,
            .offset     = 0,
            .size       = tempData.sizeInBytes
        });

        if (_device->Capabilities().hasHardwareRaytracing)
        {
            BuildBLAS(outData, _device, uploadCL, &_allocator, dataBuffer, desc.identifier);
        }

        return outData;
    }

    namespace
    {
        void DestroyVertexStream(rhi::Device* device, const VertexStream& stream)
        {
            if (stream.view != rhi::BufferView::Invalid)
            {
                device->Destroy(stream.view);
            }
        }
    }

    void GeometryBuilder::Destroy(GeometryData& data)
    {
        for (const GeometryPart& part : data.parts)
        {
            _device->Destroy(part.indices);
        }

        if (!data.parts.empty())
        {
            TrackedDeleteArray(data.parts.data());
        }

        DestroyVertexStream(_device, data.positions);
        DestroyVertexStream(_device, data.normals);
        DestroyVertexStream(_device, data.tangents);
        DestroyVertexStream(_device, data.colors);
        DestroyVertexStream(_device, data.uvs);

        if (data.blas != rhi::BLASView::Invalid)
        {
            _device->Destroy(data.blas);
            _device->Destroy(data.blasBuffer);
            _allocator.Free(data.blasGPURegion);
        }

        if (data.dataBuffer != rhi::Buffer::Invalid)
        {
            _device->Destroy(data.dataBuffer);
        }

        if (data.dataGPURegion.allocation != rhi::GPUAllocation::Invalid)
        {
            _allocator.Free(data.dataGPURegion);
        }
    }

    void GeometryBuilder::Finalize()
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

    rhi::CommandList* GeometryBuilder::GetCommandListForCurrentThread()
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