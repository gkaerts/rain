#pragma once

#include "common/common.hpp"
#include "common/math/math.hpp"
#include "rhi/handles.hpp"

namespace rn::rhi
{
    enum class TextureFormat : uint32_t;
    enum class RenderTargetFormat : uint32_t;
    enum class DepthFormat : uint32_t;

    enum class GPUAllocationFlags : uint32_t
	{
		None = 0x0,

		DeviceAccessOptimal 	= 0x01,
		HostVisible 			= 0x02,
		HostCoherent 			= 0x04,
		HostCached 				= 0x08,

		Memoryless 				= 0x10,

		DeviceOnly 				= DeviceAccessOptimal,
		DeviceOnlyMemoryless 	= DeviceAccessOptimal | Memoryless,
		HostUpload				= HostVisible | HostCoherent | HostCached,
		HostReadback			= HostVisible | HostCached
	};
	RN_DEFINE_ENUM_CLASS_BITWISE_API(GPUAllocationFlags)

	enum class BufferCreationFlags : uint32_t
	{
		None = 0x0,
		AllowShaderReadOnly = 0x01,
		AllowShaderReadWrite = 0x02,
		AllowAccelerationStructure = 0x04,
		AllowUniformBuffer = 0x08,
	};
	RN_DEFINE_ENUM_CLASS_BITWISE_API(BufferCreationFlags)

	enum class TextureCreationFlags : uint32_t
	{
		None = 0x0,
		AllowShaderReadOnly = 0x01,
		AllowShaderReadWrite = 0x02,
		AllowRenderTarget = 0x04,
		AllowDepthStencilTarget = 0x08,
		AllowCubemap = 0x10,
	};
	RN_DEFINE_ENUM_CLASS_BITWISE_API(TextureCreationFlags)

	enum class IndexFormat : uint32_t
	{
		Uint16,
		Uint32
	};

	enum class PositionFormat : uint32_t
	{
		XYZFloat,
	};

	struct GPUMemoryRegion
	{
		GPUAllocation allocation;
		size_t offsetInAllocation;
		size_t regionSize;
	};

	struct ResourceFootprint
	{
		uint64_t sizeInBytes;
		uint64_t alignment;
	};

	struct MipUploadDesc
	{
		uint64_t offsetInUploadBuffer;
		TextureFormat format;
		uint32_t width;
		uint32_t height;
		uint32_t depth;
		uint32_t rowPitch;
		uint32_t rowCount;
		uint64_t rowSizeInBytes;
		uint64_t totalSizeInBytes;
	};

	struct ASFootprint
	{
		uint64_t destDataSizeInBytes;
		uint64_t scratchDataSizeInBytes;
		uint64_t updateDataSizeInBytes;
	};

	struct BLASTriangleGeometryDesc
	{
		Buffer indexBuffer;
		uint32_t offsetInIndexBuffer;
		uint32_t indexCount;
		IndexFormat indexFormat;
		Buffer vertexBuffer;
		uint32_t offsetInVertexBuffer;
		uint32_t vertexCount;
		uint32_t vertexStride;
		PositionFormat positionFormat;
	};

	enum class ShaderRecordTableFlags
	{
		None = 0,

		ContainsRayGenShader = 0x01,
	};
	RN_DEFINE_ENUM_CLASS_BITWISE_API(ShaderRecordTableFlags);

	struct SRTFootprintDesc
	{
		ShaderRecordTableFlags flags;
		uint32_t missShaderCount;
		uint32_t hitGroupCount;
	};

	struct SRTFootprint
	{
		size_t bufferSize;

		uint32_t rayGenRecordOffset;
		uint32_t rayGenRecordSize;

		uint32_t missRecordOffset;
		uint32_t missRecordBufferSize;
		uint32_t missRecordStride;

		uint32_t hitGroupRecordOffset;
		uint32_t hitGroupRecordBufferSize;
		uint32_t hitGroupRecordStride;
	};

	struct ClearValue
	{
		union
		{
			float color[4];
			struct
			{
				float depth;
				uint8_t stencil;
			} depthStencil;
		};
	};

    ClearValue ClearColor(float r, float g, float b, float a);
    ClearValue ClearDepthStencil(float depth, uint8_t stencil);

    struct BufferDesc
    {
        BufferCreationFlags flags;
        const char* name;
    };

    struct Texture2DDesc
    {
        TextureCreationFlags flags;
        uint32_t width;
        uint32_t height;
        uint32_t arraySize;
        uint32_t mipLevels;
        TextureFormat format;
        const ClearValue* optClearValue;
        const char* name;
    };

    struct Texture3DDesc
    {
        TextureCreationFlags flags;
        uint32_t width;
		uint32_t height;
		uint32_t arraySize;
		uint32_t mipLevels;
		TextureFormat format;
		const ClearValue* optClearValue;
		const char* nam;
    };

    struct RenderTargetViewDesc
    {
        Texture2D texture;
        RenderTargetFormat format;
    };

    struct DepthStencilViewDesc
    {
        Texture2D texture;
        DepthFormat format;
    };

    struct Texture2DViewDesc
    {
        Texture2D texture;
        TextureFormat format;
        uint32_t mipCount;
        float minLODClamp;
    };

    struct Texture3DViewDesc
    {
        Texture3D texture;
        TextureFormat format;
        uint32_t mipCount;
        float minLODClamp;
    };

    struct Texture2DArrayViewDesc
    {
        Texture2D texture;
        TextureFormat format;
        uint32_t mipCount;
        uint32_t arraySize;
        float minLODClamp;
    };

    struct BufferViewDesc
    {
        Buffer buffer;
        uint32_t offsetInBytes;
        uint32_t sizeInBytes;
    };

    struct TypedBufferViewDesc
    {
        Buffer buffer;
        uint32_t offsetInBytes;
        uint32_t elementSizeInBytes;
        uint32_t elementCount;
    };

    struct UniformBufferViewDesc
    {
        Buffer buffer;
        uint32_t offsetInBytes;
        uint32_t sizeInBytes;
    };

    struct RWTexture2DViewDesc
    {
        Texture2D texture;
        TextureFormat format;
        uint32_t mipIndex;
    };

    struct RWTexture3DViewDesc
    {
        Texture3D texture;
        TextureFormat format;
        uint32_t mipIndex;
    };

    struct RWTexture2DArrayViewDesc
    {
        Texture2D texture;
        TextureFormat format;
        uint32_t mipIndex;
        uint32_t arraySize;
    };

    struct RWBufferViewDesc
    {
        Buffer buffer;
        uint32_t offsetInBytes;
        uint32_t sizeInBytes;
    };

    struct ASViewDesc
    {
        Buffer asBuffer;
        uint32_t offsetInBytes;
        uint32_t sizeInBytes;
    };

    struct TLASInstanceDesc
    {
        float4x4 transform;
        uint8_t instanceMask;
        uint32_t hitGroupModifier;
        uint32_t instanceID;

        Buffer blasBuffer;
        uint32_t offsetInBlasBuffer;
    };
}