#include "data/geometry.hpp"
#include "data/schema/geometry_generated.h"

#include "rhi/device.hpp"

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

        rhi::IndexFormat ToIndexFormat(schema::render::IndexElementFormat indexFormat)
        {
            return rhi::IndexFormat(indexFormat);
        }

        VertexStreamFormat ToVertexStreamFormat(schema::render::VertexStreamFormat format)
        {
            return VertexStreamFormat(format);
        }

        VertexStream MakeVertexStream(rhi::Device* device, rhi::Buffer dataBuffer, const schema::render::VertexStream* inStream)
        {
            return {
                .view = device->CreateBufferView({
                        .buffer = dataBuffer,
                        .offsetInBytes = inStream->region().offset_in_bytes(),
                        .sizeInBytes = inStream->region().size_in_bytes()
                    }),
                .format = ToVertexStreamFormat(inStream->format()),
                .componentCount = inStream->component_count(),
                .elementStride = inStream->stride(),
                .offsetInDataBuffer = inStream->region().offset_in_bytes()
            };
        }

        uint32_t CalculateIndexCount(schema::render::IndexElementFormat indexFormat, size_t bufferSize)
        {
            constexpr const size_t INDEX_SIZES[CountOf<schema::render::IndexElementFormat>()] = {
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
            const char* identifier)
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
                    .offsetInVertexBuffer   = part.positions.offsetInDataBuffer,
                    .vertexCount            = part.vertexCount,
                    .vertexStride           = part.positions.elementStride,
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
                    .toAccess   = rhi::PipelineAccess::AccelerationStructureRead,

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
        const schema::render::Geometry* asset = schema::render::GetGeometry(desc.data.data());

        rhi::GPUMemoryRegion dataRegion = _allocator.Allocate(asset->data()->size());
        rhi::Buffer dataBuffer = _device->CreateBuffer({
            .flags = rhi::BufferCreationFlags::AllowShaderReadOnly | rhi::BufferCreationFlags::AllowShaderReadWrite,
            .size = uint32_t(asset->data()->size()),
            .name = desc.identifier
        }, dataRegion);

        const auto* parts = asset->parts();
        Span<GeometryPart> outParts = { TrackedNewArray<GeometryPart>(MemoryCategory::Data, parts->size()), parts->size() };

        uint32_t partIdx = 0;
        for (const schema::render::GeometryPart* part : *parts)
        {
            const schema::render::VertexStream* positions = part->positions();
            RN_ASSERT(positions &&
                positions->region().offset_in_bytes() % GEOMETRY_REGION_ALIGNMENT == 0 &&
                positions->type() == schema::render::VertexStreamType::Position &&
                positions->format() == schema::render::VertexStreamFormat::Float &&
                positions->component_count() >= 3);


            const schema::render::BufferRegion* indices = part->indices();
            RN_ASSERT(indices &&
                indices->offset_in_bytes() % GEOMETRY_REGION_ALIGNMENT == 0);

            GeometryPart& outPart = outParts[partIdx++];
            outPart = 
            {
                .materialIndex  = part->material_idx(),
                .vertexCount    = part->positions()->region().size_in_bytes() / part->positions()->stride(),

                .positions      = MakeVertexStream(_device, dataBuffer, part->positions()),
                .indices        = _device->CreateBufferView({
                                    .buffer = dataBuffer,
                                    .offsetInBytes = indices->offset_in_bytes(),
                                    .sizeInBytes = indices->size_in_bytes()
                                }),

                .indexFormat    = ToIndexFormat(part->indices_format()),
                .indexOffset    = indices->offset_in_bytes(),
                .indexCount     = CalculateIndexCount(part->indices_format(), indices->size_in_bytes())
            };

            const auto* vertexStreams = part->vertex_streams();
            for (const schema::render::VertexStream* stream : *vertexStreams)
            {
                VertexStream* destStream = nullptr;
                switch(stream->type())
                {
                    case schema::render::VertexStreamType::Normal:
                        RN_ASSERT(outPart.normals.view == rhi::BufferView::Invalid);
                        destStream = &outPart.normals;
                    break;
                    case schema::render::VertexStreamType::Tangent:
                        RN_ASSERT(outPart.tangents.view == rhi::BufferView::Invalid);
                        destStream = &outPart.tangents;
                    break;
                    case schema::render::VertexStreamType::UV:
                        RN_ASSERT(outPart.uvs.view == rhi::BufferView::Invalid);
                        destStream = &outPart.uvs;
                    break;
                    case schema::render::VertexStreamType::Color:
                        RN_ASSERT(outPart.colors.view == rhi::BufferView::Invalid);
                        destStream = &outPart.colors;
                    break;
                }

                if (destStream)
                {
                    *destStream = MakeVertexStream(_device, dataBuffer, stream);
                }
            }
        }

        rhi::CommandList* uploadCL = GetCommandListForCurrentThread();
        rhi::TemporaryResource tempData = uploadCL->AllocateTemporaryResource(asset->data()->size());
        std::memcpy(tempData.cpuPtr, asset->data()->data(), asset->data()->size());

        uploadCL->Barrier({
            .bufferBarriers = {{
                .fromStage  = rhi::PipelineSyncStage::None,
                .toStage    = rhi::PipelineSyncStage::Copy,
                .fromAccess = rhi::PipelineAccess::None,
                .toAccess   = rhi::PipelineAccess::CopyWrite,
                .handle     = dataBuffer,
                .offset     = 0,
                .size       = tempData.sizeInBytes
            }}
        });

        uploadCL->CopyBufferRegion(dataBuffer, 0, tempData.buffer, tempData.offsetInBytes, tempData.sizeInBytes);

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

        GeometryData outData = 
        {
            .dataGPURegion = dataRegion,
            .dataBuffer = dataBuffer,
            .parts = outParts,
            .aabb = {
                .min = { asset->aabb()->min().x(), asset->aabb()->min().y(), asset->aabb()->min().z() },
                .max = { asset->aabb()->max().x(), asset->aabb()->max().y(), asset->aabb()->max().z() },
            }
        };

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
            DestroyVertexStream(_device, part.positions);
            DestroyVertexStream(_device, part.normals);
            DestroyVertexStream(_device, part.tangents);
            DestroyVertexStream(_device, part.colors);
            DestroyVertexStream(_device, part.uvs);
        }
        TrackedDeleteArray(data.parts.data());

        if (data.blas != rhi::BLASView::Invalid)
        {
            _device->Destroy(data.blas);
            _device->Destroy(data.blasBuffer);
            _allocator.Free(data.blasGPURegion);
        }

        _device->Destroy(data.dataBuffer);
        _allocator.Free(data.dataGPURegion);
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

}