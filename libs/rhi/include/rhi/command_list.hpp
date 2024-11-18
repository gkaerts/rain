#pragma once

#include "common/common.hpp"
#include "common/memory/span.hpp"
#include "rhi/handles.hpp"
#include "rhi/resource.hpp"
#include "rhi/temporary_resource.hpp"

namespace rn::rhi
{
    enum class IndexFormat : uint32_t;

    enum class LoadOp : uint32_t
	{
		DoNotCare = 0,

		Load,
		Clear
	};

    enum class ShadingRate : uint32_t
	{
		SR_1X1,
		SR_1X2,
		SR_2X1,
		SR_2X2,
		SR_2X4,
		SR_4X2,
		SR_4X4
	};

    enum class PipelineSyncStage : uint32_t
	{
		None = 0,

		// Common
		IndirectCommand				= 0x01,

		// Graphics pipeline
		InputAssembly				= 0x02,
		VertexShader				= 0x04,
		PixelShader 				= 0x08,
		EarlyDepthTest				= 0x10,
		LateDepthTest				= 0x20,
		RenderTargetOutput 			= 0x40,

		// Compute pipeline
		ComputeShader 				= 0x80,

		// Ray tracing pipeline
		RayTracing 					= 0x100,
		BuildAccelerationStructure	= 0x200,
		CopyAccelerationStructure	= 0x400,

		// Copy pipeline
		Copy 						= 0x800,

		All							= 0xFFFFFFFF
	};
	RN_DEFINE_ENUM_CLASS_BITWISE_API(PipelineSyncStage)

    enum class PipelineAccess : uint32_t
	{
		None = 0,

		CommandInput				= 0x01,
		VertexInput					= 0x02,
		IndexInput					= 0x04,
		ShaderRead					= 0x08,
		ShaderReadWrite				= 0x10,
		RenderTargetWrite			= 0x20,
		DepthTargetRead				= 0x40,
		DepthTargetReadWrite		= 0x80,
		CopyRead					= 0x100,
		CopyWrite					= 0x200,
		AccelerationStructureRead	= 0x400,
		AccelerationStructureWrite	= 0x800,
		UniformBuffer				= 0x1000
	};
	RN_DEFINE_ENUM_CLASS_BITWISE_API(PipelineAccess)

    enum class TextureLayout : uint32_t
	{
		Undefined = 0,

		General,

		RenderTarget,
		DepthTargetRead,
		DepthTargetReadWrite,
		ShaderRead,
		ShaderReadWrite,
		CopyRead,
		CopyWrite,
		Present,

        Count
	};

    struct PipelineBarrier
	{
		PipelineSyncStage fromStage;
		PipelineSyncStage toStage;

		PipelineAccess fromAccess;
		PipelineAccess toAccess;
	};

    struct BufferBarrier
	{
		PipelineSyncStage fromStage;
		PipelineSyncStage toStage;

		PipelineAccess fromAccess;
		PipelineAccess toAccess;

		Buffer handle;
		uint32_t offset;
		uint32_t size;
	};

    struct Texture2DBarrier
	{
		PipelineSyncStage fromStage;
		PipelineSyncStage toStage;

		PipelineAccess fromAccess;
		PipelineAccess toAccess;

		TextureLayout fromLayout;
		TextureLayout toLayout;

		Texture2D handle;
		
		uint32_t firstMipLevel;
		uint32_t numMips;

		uint32_t firstArraySlice;
		uint32_t numArraySlices;
	};

    struct Texture3DBarrier
	{
		PipelineSyncStage fromStage;
		PipelineSyncStage toStage;

		PipelineAccess fromAccess;
		PipelineAccess toAccess;

		TextureLayout fromLayout;
		TextureLayout toLayout;

		Texture3D handle;
		
		uint32_t firstMipLevel;
		uint32_t numMips;
	};


    struct DrawPacket
	{
		RasterPipeline pipeline;

		uint32_t vertexCount;
		uint32_t instanceCount;
		uint32_t drawID;
	};

    struct IndexedDrawPacket
	{
		RasterPipeline pipeline;

		Buffer indexBuffer;
		uint32_t offsetInIndexBuffer;
		IndexFormat indexFormat;

		uint32_t indexCount;
		uint32_t instanceCount;
		uint32_t drawID;
	};

    struct IndirectDrawPacket
	{
		RasterPipeline pipeline;

		Buffer argsBuffer;
		uint32_t offsetInArgsBuffer;
	};

    struct IndirectIndexedDrawPacket
	{
		RasterPipeline pipeline;

		Buffer indexBuffer;
		uint32_t offsetInIndexBuffer;
		uint32_t indexBufferSizeInBytes;
		IndexFormat indexFormat;

		Buffer argsBuffer;
		uint32_t offsetInArgsBuffer;
	};

    struct DispatchPacket
	{
		ComputePipeline pipeline;

		uint32_t x;
		uint32_t y;
		uint32_t z;
	};

    struct IndirectDispatchPacket
	{
		ComputePipeline pipeline;

		Buffer argsBuffer;
		uint32_t offsetInArgsBuffer;
	};

    struct ShaderRecordBuffer
	{
		Buffer recordBuffer;
		uint32_t offsetInRecordBuffer;
		uint32_t bufferSize;
		uint32_t recordStride;
	};

    struct DispatchRaysPacket
	{
		RTPipeline pipeline;

		ShaderRecordBuffer rayGenBuffer;
		ShaderRecordBuffer missBuffer;
		ShaderRecordBuffer hitGroupBuffer;
		
		uint32_t x;
		uint32_t y;
		uint32_t z;
	};

    struct IndirectDispatchRaysPacket
	{
		RTPipeline pipeline;
		
		Buffer argsBuffer;
		uint32_t offsetInArgsBuffer;
	};

    struct DispatchMeshPacket
	{
		RasterPipeline pipeline;

		uint32_t x;
		uint32_t y;
		uint32_t z;
	};

    struct IndirectDispatchMeshPacket
	{
		RasterPipeline pipeline;

		Buffer argsBuffer;
		uint32_t offsetInArgsBuffer;
	};

    struct Viewport
	{
		uint32_t x;
		uint32_t y;
		uint32_t width;
		uint32_t height;
	};

    struct Rect
	{
		uint32_t x;
		uint32_t y;
		uint32_t width;
		uint32_t height;
	};

    struct ShaderRecordBuildDesc
	{
		const char* shaderName;
		Buffer uniformDataBuffer;
		uint32_t offsetInUniformDataBuffer;
	};

    struct RenderPassRenderTarget
    {
        RenderTargetView view;
        LoadOp loadOp;
        ClearValue clearValue;
    };

    struct RenderPassDepthTarget
    {
        DepthStencilView view;
        LoadOp loadOp;
        ClearValue clearValue;
    };
    struct RenderPassBeginDesc
    {
        Viewport viewport;
        Span<const RenderPassRenderTarget> renderTargets;
        RenderPassDepthTarget depthTarget;
    };

    struct BarrierDesc
    {
        Span<const PipelineBarrier> pipelineBarriers;
        Span<const BufferBarrier> bufferBarriers;
        Span<const Texture2DBarrier> texture2DBarriers;
        Span<const Texture3DBarrier> texture3DBarriers;
    };

    struct BLASBuildDesc
    {
        BLASView blas;
        Buffer blasBuffer;
        uint32_t offsetInBlasBuffer;

        Buffer scratchBuffer;
        uint32_t offsetInScratchBuffer;

        Span<const BLASTriangleGeometryDesc> geometry;
    };

    struct TLASBuildDesc
    {
        TLASView tlas;
        Buffer tlasBuffer;
        uint32_t offetInTlasBuffer;

        Buffer scratchBuffer;
        uint32_t offsetInScratchBuffer;

        Buffer instanceBuffer;
        uint32_t offsetInInstanceBuffer;
        uint32_t instanceCount;
    };

    struct SRTBuildDesc
    {
        Buffer srtBuffer;
        uint32_t offsetInSRTBuffer;

        RTPipeline rtPipeline;

        SRTFootprint srtFootprint;
        ShaderRecordBuildDesc rayGenDesc;
        Span<const ShaderRecordBuildDesc> missDescs;
        Span<const ShaderRecordBuildDesc> hitGroupDescs;
    };

    using FnOnReadback = void(*)(Span<const unsigned char> data, void* userData);
    class CommandList
    {
    public:

        virtual ~CommandList() {};

        virtual void BeginEvent(const char* fmt) { RN_NOT_IMPLEMENTED(); }
        virtual void EndEvent() { RN_NOT_IMPLEMENTED(); }

        virtual void BeginRenderPass(const RenderPassBeginDesc& desc) { RN_NOT_IMPLEMENTED(); }
        virtual void EndRenderPass() { RN_NOT_IMPLEMENTED(); }

        virtual void SetScissorRect(const Rect& rect) { RN_NOT_IMPLEMENTED(); }
        virtual void SetShadingRate(ShadingRate rate) { RN_NOT_IMPLEMENTED(); }

        virtual void PushGraphicsConstantData(uint32_t offsetInData, const void* data, uint32_t dataSize) { RN_NOT_IMPLEMENTED(); }
        virtual void PushComputeConstantData(uint32_t offsetInData, const void* data, uint32_t dataSize) { RN_NOT_IMPLEMENTED(); }

        template <typename T>
        void PushGraphicsConstants(uint32_t offset, const T& data) { PushGraphicsConstantData(offset, &data, sizeof(data)); }

        template <typename T>
        void PushComputeConstants(uint32_t offset, const T& data) { PushComputeConstantData(offset, &data, sizeof(data)); }

        virtual void BindDrawIDBuffer(Buffer drawIDBuffer, uint32_t offsetInBuffer, uint32_t drawIDCount) { RN_NOT_IMPLEMENTED(); }

        virtual void Draw(Span<const DrawPacket> packets) { RN_NOT_IMPLEMENTED(); }
        virtual void DrawIndexed(Span<const IndexedDrawPacket> packets) { RN_NOT_IMPLEMENTED(); }
        virtual void DrawIndirect(Span<const IndirectDrawPacket> packets) { RN_NOT_IMPLEMENTED(); }
        virtual void DrawIndirectIndexed(Span<const IndirectIndexedDrawPacket> packets) { RN_NOT_IMPLEMENTED(); }
        virtual void Dispatch(Span<const DispatchPacket> packets) { RN_NOT_IMPLEMENTED(); }
        virtual void DispatchIndirect(Span<const IndirectDispatchPacket> packets) { RN_NOT_IMPLEMENTED(); }
        virtual void DispatchMesh(Span<const DispatchMeshPacket> packets) { RN_NOT_IMPLEMENTED(); }
        virtual void DispatchMeshIndirect(Span<const IndirectDispatchMeshPacket> packets) { RN_NOT_IMPLEMENTED(); }
        virtual void DispatchRays(Span<const DispatchRaysPacket> packets) { RN_NOT_IMPLEMENTED(); }
        virtual void DispatchRaysIndirect(Span<const IndirectDispatchRaysPacket> packets) { RN_NOT_IMPLEMENTED(); }

        virtual void Barrier(const BarrierDesc& desc) { RN_NOT_IMPLEMENTED(); }

        virtual TemporaryResource AllocateTemporaryResource(uint32_t sizeInBytes) { RN_NOT_IMPLEMENTED(); return {}; }

        virtual void UploadBufferData(Buffer destBuffer, uint32_t destBufferOffset, Span<const unsigned char> data) { RN_NOT_IMPLEMENTED(); }

        template <typename T>
        void UploadTypedBufferData(Buffer destBuffer, uint32_t destBufferOffset, Span<const T> data)
        {
            Span<const unsigned char> byteData(reinterpret_cast<const unsigned char*>(data.data()), data.size() * sizeof(T));
            UploadBufferData(destBuffer, destBufferOffset, byteData);
        }

        virtual void UploadTextureData(Texture2D destTexture, uint32_t startMipIndex, Span<const MipUploadDesc> mipDescs, Span<const unsigned char> sourceData) { RN_NOT_IMPLEMENTED(); }
        virtual void UploadTextureData(Texture3D destTexture, uint32_t startMipIndex, Span<const MipUploadDesc> mipDescs, Span<const unsigned char> sourceData) { RN_NOT_IMPLEMENTED(); }

        virtual void CopyBufferRegion(Buffer dest, uint32_t destOffsetInBytes, Buffer src, uint32_t srcOffsetInBytes, uint32_t sizeInBytes) { RN_NOT_IMPLEMENTED(); }
        virtual void CopyTexture(Texture2D dest, Texture2D src) { RN_NOT_IMPLEMENTED(); }
        virtual void CopyTexture(Texture3D dest, Texture3D src) { RN_NOT_IMPLEMENTED(); }

        virtual void UploadTLASInstances(Buffer instanceBuffer, uint32_t offsetInInstanceBuffer, Span<const TLASInstanceDesc> instances) { RN_NOT_IMPLEMENTED(); }
        virtual void BuildBLAS(const BLASBuildDesc& desc) { RN_NOT_IMPLEMENTED(); }
        virtual void BuildTLAS(const TLASBuildDesc& desc) { RN_NOT_IMPLEMENTED(); }
        virtual void BuildShaderRecordTable(const SRTBuildDesc& desc) { RN_NOT_IMPLEMENTED(); }

        virtual void QueueBufferReadback(Buffer buffer, uint32_t offsetInBuffer, uint32_t sizeInBytes, FnOnReadback onReadback, void* userData) { RN_NOT_IMPLEMENTED(); };
        virtual void QueueTextureReadback(Texture2D texture, FnOnReadback onReadback, void* userData) { RN_NOT_IMPLEMENTED(); };
        virtual void QueueTextureReadback(Texture3D texture, FnOnReadback onReadback, void* userData) { RN_NOT_IMPLEMENTED(); };

    };

	struct GPUEventScope
	{
		GPUEventScope(CommandList* cl, const char* fmt)
		 : _cl(cl)
		{
			cl->BeginEvent(fmt);
		}

		~GPUEventScope()
		{
			_cl->EndEvent();
		}

		GPUEventScope(const GPUEventScope&) = delete;
		GPUEventScope(GPUEventScope&&) = delete;
		GPUEventScope& operator=(const GPUEventScope&) = delete;
		GPUEventScope& operator=(GPUEventScope&&) = delete;

		CommandList* _cl = nullptr;
	};

	#define RN_GPU_EVENT_SCOPE(cl, fmt) rn::rhi::GPUEventScope _gpuEventScope_##__LINE__(cl, fmt);
}