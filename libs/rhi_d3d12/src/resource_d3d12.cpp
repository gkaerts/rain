#define NOMINMAX
#include "resource_d3d12.hpp"
#include "device_d3d12.hpp"
#include "format_d3d12.hpp"
#include "rhi/format.hpp"
#include "d3d12.h"

namespace rn::rhi
{
    RN_MEMORY_CATEGORY(RHI)

    namespace
    {
        constexpr const uint32_t BINDLESS_RANGE_SIZE = 100000;
        constexpr const uint32_t MAX_SAMPLER_DESCRIPTORS = 64;
        constexpr const uint32_t MAX_RTV_DESCRIPTORS = 2048;
        constexpr const uint32_t MAX_DSV_DESCRIPTORS = 2048;
    }

    DescriptorHeap::DescriptorHeap()
        : _indexAllocator(MemoryCategory::RHI, 0)
    {}

    DescriptorHeap::~DescriptorHeap()
    {
        if (_heap)
        {
            _heap->Release();
            _heap = nullptr;
        }    
    }

    void DescriptorHeap::InitAsResourceHeap(ID3D12Device10* device)
    {
        RN_ASSERT(_heap == nullptr);
        D3D12_DESCRIPTOR_HEAP_DESC desc = {
            .Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
            .NumDescriptors = BINDLESS_RANGE_SIZE,
            .Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE,
        };

        RN_ASSERT(SUCCEEDED(device->CreateDescriptorHeap(&desc,
            __uuidof(ID3D12DescriptorHeap),
            (void**)(&_heap))));

        _descriptorSizeInBytes = device->GetDescriptorHandleIncrementSize(desc.Type);
        _indexAllocator = IndexAllocator(MemoryCategory::RHI, BINDLESS_RANGE_SIZE);
        _type = Type::Resource;
    }

    void DescriptorHeap::InitAsSamplerHeap(ID3D12Device10* device)
    {
        RN_ASSERT(_heap == nullptr);
        D3D12_DESCRIPTOR_HEAP_DESC desc = {
            .Type = D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER,
            .NumDescriptors = MAX_SAMPLER_DESCRIPTORS,
            .Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE,
        };

        RN_ASSERT(SUCCEEDED(device->CreateDescriptorHeap(&desc,
            __uuidof(ID3D12DescriptorHeap),
            (void**)(&_heap))));

        _descriptorSizeInBytes = device->GetDescriptorHandleIncrementSize(desc.Type);
        _indexAllocator = IndexAllocator(MemoryCategory::RHI, MAX_SAMPLER_DESCRIPTORS);
        _type = Type::Sampler;
    }

    void DescriptorHeap::InitAsRTVHeap(ID3D12Device10* device)
    {
        RN_ASSERT(_heap == nullptr);
        D3D12_DESCRIPTOR_HEAP_DESC desc = {
            .Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV,
            .NumDescriptors = MAX_RTV_DESCRIPTORS,
            .Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE,
        };

        RN_ASSERT(SUCCEEDED(device->CreateDescriptorHeap(&desc,
            __uuidof(ID3D12DescriptorHeap),
            (void**)(&_heap))));

        _descriptorSizeInBytes = device->GetDescriptorHandleIncrementSize(desc.Type);
        _indexAllocator = IndexAllocator(MemoryCategory::RHI, MAX_RTV_DESCRIPTORS);
        _type = Type::RTV;
    }

    void DescriptorHeap::InitAsDSVHeap(ID3D12Device10* device)
    {
        RN_ASSERT(_heap == nullptr);
        D3D12_DESCRIPTOR_HEAP_DESC desc = {
            .Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV,
            .NumDescriptors = MAX_DSV_DESCRIPTORS,
            .Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE,
        };

        RN_ASSERT(SUCCEEDED(device->CreateDescriptorHeap(&desc,
            __uuidof(ID3D12DescriptorHeap),
            (void**)(&_heap))));

        _descriptorSizeInBytes = device->GetDescriptorHandleIncrementSize(desc.Type);
        _indexAllocator = IndexAllocator(MemoryCategory::RHI, MAX_DSV_DESCRIPTORS);
        _type = Type::DSV;
    }

    ResourceDescriptor DescriptorHeap::AllocateDescriptor()
    {
        return ResourceDescriptor(_indexAllocator.Allocate());
    }

    void DescriptorHeap::FreeDescriptor(ResourceDescriptor descriptor)
    {
        _indexAllocator.Free(ResourceIndex(descriptor));
    }

    void DescriptorHeap::FillDescriptor(ID3D12Device10* device, ResourceDescriptor descriptor, ID3D12Resource* resource, const D3D12_SHADER_RESOURCE_VIEW_DESC& desc)
    {
        RN_ASSERT(_type == Type::Resource);
        UINT offsetInHeap = _descriptorSizeInBytes * UINT(descriptor);
        D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle = _heap->GetCPUDescriptorHandleForHeapStart();
        cpuHandle.ptr += offsetInHeap;

        device->CreateShaderResourceView(
            resource,
            &desc,
            cpuHandle);
    }

    void DescriptorHeap::FillDescriptor(ID3D12Device10* device, ResourceDescriptor descriptor, ID3D12Resource* resource, const D3D12_UNORDERED_ACCESS_VIEW_DESC& desc)
    {
        RN_ASSERT(_type == Type::Resource);
        UINT offsetInHeap = _descriptorSizeInBytes * UINT(descriptor);
        D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle = _heap->GetCPUDescriptorHandleForHeapStart();
        cpuHandle.ptr += offsetInHeap;

        device->CreateUnorderedAccessView(
            resource,
            nullptr,
            &desc,
            cpuHandle);
    }

    void DescriptorHeap::FillDescriptor(ID3D12Device10* device, ResourceDescriptor descriptor, const D3D12_CONSTANT_BUFFER_VIEW_DESC& desc)
    {
        RN_ASSERT(_type == Type::Resource);
        UINT offsetInHeap = _descriptorSizeInBytes * UINT(descriptor);
        D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle = _heap->GetCPUDescriptorHandleForHeapStart();
        cpuHandle.ptr += offsetInHeap;

        device->CreateConstantBufferView(
            &desc,
            cpuHandle);
    }

    void DescriptorHeap::FillDescriptor(ID3D12Device10* device, ResourceDescriptor descriptor, ID3D12Resource* resource, const D3D12_RENDER_TARGET_VIEW_DESC& desc)
    {
        RN_ASSERT(_type == Type::RTV);
        UINT offsetInHeap = _descriptorSizeInBytes * UINT(descriptor);
        D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle = _heap->GetCPUDescriptorHandleForHeapStart();
        cpuHandle.ptr += offsetInHeap;

        device->CreateRenderTargetView(
            resource,
            &desc,
            cpuHandle);
    }

    void DescriptorHeap::FillDescriptor(ID3D12Device10* device, ResourceDescriptor descriptor, ID3D12Resource* resource, const D3D12_DEPTH_STENCIL_VIEW_DESC& desc)
    {
        RN_ASSERT(_type == Type::DSV);
        UINT offsetInHeap = _descriptorSizeInBytes * UINT(descriptor);
        D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle = _heap->GetCPUDescriptorHandleForHeapStart();
        cpuHandle.ptr += offsetInHeap;

        device->CreateDepthStencilView(
            resource,
            &desc,
            cpuHandle);
    }

    void DescriptorHeap::FillDescriptor(ID3D12Device10* device, ResourceDescriptor descriptor, const D3D12_SAMPLER_DESC& desc)
    {
        RN_ASSERT(_type == Type::Sampler);
        UINT offsetInHeap = _descriptorSizeInBytes * UINT(descriptor);
        D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle = _heap->GetCPUDescriptorHandleForHeapStart();
        cpuHandle.ptr += offsetInHeap;

        device->CreateSampler(
            &desc,
            cpuHandle);
    }

    D3D12_CPU_DESCRIPTOR_HANDLE DescriptorHeap::Resolve(ResourceDescriptor descriptor) const
    {
        D3D12_CPU_DESCRIPTOR_HANDLE handle = _heap->GetCPUDescriptorHandleForHeapStart();
        handle.ptr += uint32_t(descriptor) * _descriptorSizeInBytes;
        
        return handle;
    }

    namespace
    {
        constexpr const D3D12_HEAP_DESC HeapDescFromAllocationFlags(uint64_t sizeInBytes, bool isUMA,  GPUAllocationFlags flags)
        {
            D3D12_HEAP_DESC heapDesc = {
                .SizeInBytes = sizeInBytes
            };

            if (isUMA)
            {
                // We're guaranteed to have cache-coherent UMA. We can ignore the device/host-local aspects of gpu_allocation_flags and always allocate out of L0
                heapDesc.Properties = {
                    .Type = D3D12_HEAP_TYPE_CUSTOM,
                    .CPUPageProperty = 
                        (TestFlag(flags, GPUAllocationFlags::HostCoherent) && !TestFlag(flags, GPUAllocationFlags::HostCached)) ? D3D12_CPU_PAGE_PROPERTY_WRITE_COMBINE :
                        (TestFlag(flags, GPUAllocationFlags::HostCached)) ? D3D12_CPU_PAGE_PROPERTY_WRITE_BACK :
                        D3D12_CPU_PAGE_PROPERTY_UNKNOWN,
                    .MemoryPoolPreference = D3D12_MEMORY_POOL_L0
                };
            }
            else
            {
                // Check whether we can match some of the built-in D3D12 heap types before we create a custom specification
                if (flags == GPUAllocationFlags::DeviceOnly)
                {
                    heapDesc.Properties = {
                        .Type = D3D12_HEAP_TYPE_DEFAULT
                    };
                }
                else if (flags == GPUAllocationFlags::HostUpload)
                {
                    heapDesc.Properties = {
                        .Type = D3D12_HEAP_TYPE_UPLOAD
                    };
                }
                else if (flags == GPUAllocationFlags::HostReadback)
                {
                    heapDesc.Properties = {
                        .Type = D3D12_HEAP_TYPE_READBACK,
                    };
                }
                else
                {
                    // We're doing something more exotic. Create a custom heap. No guarantee that this will actually succeed.
                    heapDesc.Properties = {
                        .Type = D3D12_HEAP_TYPE_CUSTOM,
                        .CPUPageProperty = 
                            (TestFlag(flags, GPUAllocationFlags::HostCoherent) && !TestFlag(flags, GPUAllocationFlags::HostCached)) ? D3D12_CPU_PAGE_PROPERTY_WRITE_COMBINE :
                            (TestFlag(flags, GPUAllocationFlags::HostCached)) ? D3D12_CPU_PAGE_PROPERTY_WRITE_BACK :
                            D3D12_CPU_PAGE_PROPERTY_UNKNOWN,
                        .MemoryPoolPreference = TestFlag(flags, GPUAllocationFlags::DeviceAccessOptimal) ? D3D12_MEMORY_POOL_L0 : D3D12_MEMORY_POOL_L1
                    };
                }
            }

            return heapDesc;
        }

        // {595A5D8B-EBC7-4DDE-A851-B562D9F7D7F5}
        static const GUID MEMORY_CATEGORY_GUID = { 0x595a5d8b, 0xebc7, 0x4dde, { 0xa8, 0x51, 0xb5, 0x62, 0xd9, 0xf7, 0xd7, 0xf5 } };
    }

    GPUAllocation DeviceD3D12::GPUAlloc(MemoryCategoryID cat, uint64_t sizeInBytes, GPUAllocationFlags flags)
    {
        D3D12_HEAP_DESC heapDesc = HeapDescFromAllocationFlags(sizeInBytes, _caps.isUMA, flags);

        ID3D12Heap* heap = nullptr;
        RN_ASSERT(SUCCEEDED(_d3dDevice->CreateHeap(&heapDesc, __uuidof(ID3D12Heap), (void**)(&heap))));

        heap->SetPrivateData(MEMORY_CATEGORY_GUID, sizeof(MemoryCategoryID), &cat);
        TrackExternalAllocation(cat, heapDesc.SizeInBytes);

        return _gpuAllocations.Store(std::move(heap));
    }

    void DeviceD3D12::GPUFree(GPUAllocation allocation)
    {
        QueueFrameFinalizerAction([](DeviceD3D12* device, void* data)
            {
                GPUAllocation a = GPUAllocation(ptrdiff_t(data));

                ID3D12Heap* heap = device->_gpuAllocations.GetHot(a);      
                if (heap)
                {
                    D3D12_HEAP_DESC heapDesc = heap->GetDesc();
                    MemoryCategoryID cat = MemoryCategoryID(0);

                    UINT catSize = sizeof(cat);
                    heap->GetPrivateData(MEMORY_CATEGORY_GUID, &catSize, &cat);
                    TrackExternalFree(cat, heapDesc.SizeInBytes);

                    heap->Release();
                }

                device->_gpuAllocations.Remove(a);
            },
            reinterpret_cast<void*>(allocation));
    }

    namespace
    {
        D3D12_RESOURCE_FLAGS ToD3D12ResourceFlags(BufferCreationFlags flags)
        {
            D3D12_RESOURCE_FLAGS d3dFlags = D3D12_RESOURCE_FLAG_NONE;
            if (TestFlag(flags, BufferCreationFlags::AllowShaderReadWrite) ||
                TestFlag(flags, BufferCreationFlags::AllowAccelerationStructure))
            {
                d3dFlags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
            }

            return d3dFlags;
        }

        D3D12_RESOURCE_FLAGS ToD3D12ResourceFlags(TextureCreationFlags flags)
        {
            D3D12_RESOURCE_FLAGS d3dFlags = D3D12_RESOURCE_FLAG_NONE;
            if (TestFlag(flags, TextureCreationFlags::AllowShaderReadWrite))
            {
                d3dFlags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
            }

            if (TestFlag(flags, TextureCreationFlags::AllowRenderTarget))
            {
                d3dFlags |= D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
            }

            if (TestFlag(flags, TextureCreationFlags::AllowDepthStencilTarget))
            {
                d3dFlags |= D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
            } 

            return d3dFlags;
        }

        void SetResourceName(ID3D12Resource* resource, std::string_view name)
        {
            wchar_t wBuffer[256]{};
            size_t numConverted = 0;
            mbstowcs_s(&numConverted, wBuffer, name.data(), std::min(RN_ARRAY_SIZE(wBuffer) - 1, name.length()));

            resource->SetName(wBuffer);
        }
    }

    Buffer DeviceD3D12::CreateBuffer(const BufferDesc& desc, const GPUMemoryRegion& region)
    {
        RN_ASSERT(IsValid(region.allocation) && region.regionSize >= desc.size);

        ID3D12Heap* heap = _gpuAllocations.GetHot(region.allocation);

        // Make sure we can actually fit inside this heap
        D3D12_HEAP_DESC heapDesc = heap->GetDesc();
        RN_ASSERT(region.offsetInAllocation + region.regionSize <= heapDesc.SizeInBytes);

        ID3D12Resource* resource = nullptr;
        HRESULT hr = S_OK;

        D3D12_RESOURCE_DESC1 resourceDesc = {
            .Dimension = D3D12_RESOURCE_DIMENSION_BUFFER,
            .Width = desc.size,
            .Height = 1,
            .DepthOrArraySize = 1,
            .MipLevels = 1,
            .SampleDesc = { 1, 0 },
            .Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR,
            .Flags = ToD3D12ResourceFlags(desc.flags),
        };

        RN_ASSERT(SUCCEEDED(_d3dDevice->CreatePlacedResource2(
            heap,
            region.offsetInAllocation,
            &resourceDesc,
            D3D12_BARRIER_LAYOUT_UNDEFINED,
            nullptr,
            0,
            nullptr,
            __uuidof(ID3D12Resource),
            (void**)&resource)));

        SetResourceName(resource, desc.name);
        
        return _buffers.Store(std::move(resource));
    }

    Texture2D DeviceD3D12::CreateTexture2D(const Texture2DDesc& desc, const GPUMemoryRegion& region)
    {
        RN_ASSERT(IsValid(region.allocation) && region.regionSize > 0);
        ID3D12Heap* heap = _gpuAllocations.GetHot(region.allocation);

        // Make sure we can actually fit inside this heap
        D3D12_HEAP_DESC heapDesc = heap->GetDesc();
        RN_ASSERT(region.offsetInAllocation + region.regionSize <= heapDesc.SizeInBytes);
        RN_ASSERT(heapDesc.Properties.Type != D3D12_HEAP_TYPE_UPLOAD && heapDesc.Properties.Type != D3D12_HEAP_TYPE_READBACK);

        D3D12_RESOURCE_FLAGS resourceFlags = ToD3D12ResourceFlags(desc.flags);
        DXGI_FORMAT dxgiFormat = ToDXGIFormat(desc.format);
        DXGI_FORMAT clearFormat = dxgiFormat;
        if ((resourceFlags & D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL))
        {
            if (!(resourceFlags & D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE))
            {
                dxgiFormat = ToTypelessDXGIFormat(desc.format);
            }
            else
            {
                dxgiFormat = ToDXGIFormat(ToDepthEquivalent(desc.format));
            }

            clearFormat = ToDXGIFormat(ToDepthEquivalent(desc.format));
        }

        D3D12_CLEAR_VALUE clearValue{};
        if (desc.optClearValue && 
            ((resourceFlags & D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL) != 0 ||
            (resourceFlags & D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET) != 0))
        {
            clearValue.Format = clearFormat;
            if ((resourceFlags & D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL))
            {
                clearValue.DepthStencil = { desc.optClearValue->depthStencil.depth, desc.optClearValue->depthStencil.stencil };
            }
            else
            {
                std::memcpy(clearValue.Color, desc.optClearValue->color, sizeof(clearValue.Color));
            }
        }

        ID3D12Resource* resource = nullptr;

        D3D12_RESOURCE_DESC1 resourceDesc = {    
            .Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D,
            .Width = desc.width,
            .Height = desc.height,
            .DepthOrArraySize = UINT16(desc.arraySize),
            .MipLevels = UINT16(desc.mipLevels),
            .Format = dxgiFormat,
            .SampleDesc = { 1, 0 },
            .Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN,
            .Flags = resourceFlags,
        };

        RN_ASSERT(SUCCEEDED(_d3dDevice->CreatePlacedResource2(
            heap,
            region.offsetInAllocation,
            &resourceDesc,
            D3D12_BARRIER_LAYOUT_UNDEFINED,
            desc.optClearValue ? &clearValue : nullptr,
            0,
            nullptr,
            __uuidof(ID3D12Resource),
            (void**)&resource)));
        
        SetResourceName(resource, desc.name);
        return _texture2Ds.Store(std::move(resource));
    }

    Texture3D DeviceD3D12::CreateTexture3D(const Texture3DDesc& desc, const GPUMemoryRegion& region)
    {
        RN_ASSERT(IsValid(region.allocation) && region.regionSize > 0);
        ID3D12Heap* heap = _gpuAllocations.GetHot(region.allocation);

        // Make sure we can actually fit inside this heap
        D3D12_HEAP_DESC heapDesc = heap->GetDesc();
        RN_ASSERT(region.offsetInAllocation + region.regionSize <= heapDesc.SizeInBytes);
        RN_ASSERT(heapDesc.Properties.Type != D3D12_HEAP_TYPE_UPLOAD && heapDesc.Properties.Type != D3D12_HEAP_TYPE_READBACK);

        D3D12_RESOURCE_DESC1 resourceDesc = {
            .Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE3D,
            .Width = desc.width,
            .Height = desc.height,
            .DepthOrArraySize = UINT16(desc.depth),
            .MipLevels = UINT16(desc.mipLevels),
            .Format = ToDXGIFormat(desc.format),
            .SampleDesc = { 1, 0 },
            .Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN,
            .Flags = ToD3D12ResourceFlags(desc.flags),
        };

        ID3D12Resource* resource = nullptr;
        RN_ASSERT(SUCCEEDED(_d3dDevice->CreatePlacedResource2(
            heap,
            region.offsetInAllocation,
            &resourceDesc,
            D3D12_BARRIER_LAYOUT_UNDEFINED,
            nullptr,
            0,
            nullptr,
            __uuidof(ID3D12Resource),
            (void**)&resource)));
        
        SetResourceName(resource, desc.name);
        return _texture3Ds.Store(std::move(resource));
    }

    RenderTargetView DeviceD3D12::CreateRenderTargetView(const RenderTargetViewDesc& rtvDesc)
    {
        D3D12_RENDER_TARGET_VIEW_DESC desc = {
            .Format = ToDXGIFormat(rtvDesc.format),
            .ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D,
            .Texture2D = { 
                .MipSlice = 0, 
                .PlaneSlice = 0 
            },
        };

        ID3D12Resource* resource = _texture2Ds.GetHot(rtvDesc.texture);

        ResourceDescriptor descriptor = _rtvDescriptorHeap.AllocateDescriptor();
        _rtvDescriptorHeap.FillDescriptor(_d3dDevice, descriptor, resource, desc);

        return AssembleHandle<RenderTargetView>(uint32_t(descriptor), 0);
    }

    DepthStencilView DeviceD3D12::CreateDepthStencilView(const DepthStencilViewDesc& dsvDesc)
    {
        D3D12_DEPTH_STENCIL_VIEW_DESC desc = {
            .Format = ToDXGIFormat(dsvDesc.format),
            .ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D,
            .Texture2D = { 
                .MipSlice = 0 
            },
        };

        ID3D12Resource* resource = _texture2Ds.GetHot(dsvDesc.texture);
        ResourceDescriptor descriptor = _dsvDescriptorHeap.AllocateDescriptor();
        _dsvDescriptorHeap.FillDescriptor(_d3dDevice, descriptor, resource, desc);

        return AssembleHandle<DepthStencilView>(uint32_t(descriptor), 0);
    }

    Texture2DView DeviceD3D12::CreateTexture2DView(const Texture2DViewDesc& desc)
    {
        D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {
            .Format = ToDXGIFormat(desc.format),
            .ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D,
            .Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING,
            .Texture2D = {
                .MostDetailedMip = 0,
                .MipLevels = desc.mipCount,
                .ResourceMinLODClamp = desc.minLODClamp,
            }
        };

        ID3D12Resource* resource = _texture2Ds.GetHot(desc.texture);
        ResourceDescriptor descriptor = _resourceDescriptorHeap.AllocateDescriptor();
        _resourceDescriptorHeap.FillDescriptor(_d3dDevice, descriptor, resource, srvDesc);

        return AssembleHandle<Texture2DView>(uint32_t(descriptor), 0);
    }

    Texture3DView DeviceD3D12::CreateTexture3DView(const Texture3DViewDesc& desc)
    {
        D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {
            .Format = ToDXGIFormat(desc.format),
            .ViewDimension = D3D12_SRV_DIMENSION_TEXTURE3D,
            .Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING,
            .Texture3D = {
                .MostDetailedMip = 0,
                .MipLevels = desc.mipCount,
                .ResourceMinLODClamp = desc.minLODClamp,
            }
        };

        ID3D12Resource* resource = _texture3Ds.GetHot(desc.texture);
        ResourceDescriptor descriptor = _resourceDescriptorHeap.AllocateDescriptor();
        _resourceDescriptorHeap.FillDescriptor(_d3dDevice, descriptor, resource, srvDesc);

        return AssembleHandle<Texture3DView>(uint32_t(descriptor), 0);
    }

    TextureCubeView DeviceD3D12::CreateTextureCubeView(const Texture2DViewDesc& desc)
    {
        D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {
            .Format = ToDXGIFormat(desc.format),
            .ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE,
            .Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING,
            .TextureCube = {
                .MostDetailedMip = 0,
                .MipLevels = desc.mipCount,
                .ResourceMinLODClamp = desc.minLODClamp,
            }
        };

        ID3D12Resource* resource = _texture2Ds.GetHot(desc.texture);
        ResourceDescriptor descriptor = _resourceDescriptorHeap.AllocateDescriptor();
        _resourceDescriptorHeap.FillDescriptor(_d3dDevice, descriptor, resource, srvDesc);

        return AssembleHandle<TextureCubeView>(uint32_t(descriptor), 0);
    }

    Texture2DArrayView DeviceD3D12::CreateTexture2DArrayView(const Texture2DArrayViewDesc& desc)
    {
        D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {
            .Format = ToDXGIFormat(desc.format),
            .ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE,
            .Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING,
            .Texture2DArray = {
                .MostDetailedMip = 0,
                .MipLevels = desc.mipCount,
                .FirstArraySlice = 0,
                .ArraySize = desc.arraySize,
                .ResourceMinLODClamp = desc.minLODClamp,
            }
        };

        ID3D12Resource* resource = _texture2Ds.GetHot(desc.texture);
        ResourceDescriptor descriptor = _resourceDescriptorHeap.AllocateDescriptor();
        _resourceDescriptorHeap.FillDescriptor(_d3dDevice, descriptor, resource, srvDesc);

        return AssembleHandle<Texture2DArrayView>(uint32_t(descriptor), 0);
    }

    BufferView DeviceD3D12::CreateBufferView(const BufferViewDesc& desc)
    {
        RN_ASSERT((desc.offsetInBytes % 4) == 0);
        RN_ASSERT((desc.sizeInBytes % 4) == 0);

        D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {
            .Format = DXGI_FORMAT_R32_TYPELESS,
            .ViewDimension = D3D12_SRV_DIMENSION_BUFFER,
            .Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING,
            .Buffer = {
                .FirstElement = desc.offsetInBytes / 4,
                .NumElements = desc.sizeInBytes / 4,
                .StructureByteStride = 0,
                .Flags  = D3D12_BUFFER_SRV_FLAG_RAW
            }
        };

        ID3D12Resource* resource = _buffers.GetHot(desc.buffer);
        ResourceDescriptor descriptor = _resourceDescriptorHeap.AllocateDescriptor();
        _resourceDescriptorHeap.FillDescriptor(_d3dDevice, descriptor, resource, srvDesc);

        return AssembleHandle<BufferView>(uint32_t(descriptor), 0);
    }

    TypedBufferView DeviceD3D12::CreateTypedBufferView(const TypedBufferViewDesc& desc)
    {
        RN_ASSERT((desc.offsetInBytes % desc.elementSizeInBytes) == 0);
        D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {
            .Format = DXGI_FORMAT_UNKNOWN,
            .ViewDimension = D3D12_SRV_DIMENSION_BUFFER,
            .Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING,
            .Buffer = {
                .FirstElement = desc.offsetInBytes / desc.elementSizeInBytes,
                .NumElements = desc.elementCount,
                .StructureByteStride = desc.elementSizeInBytes,
                .Flags  = D3D12_BUFFER_SRV_FLAG_NONE
            }
        };

        ID3D12Resource* resource = _buffers.GetHot(desc.buffer);
        ResourceDescriptor descriptor = _resourceDescriptorHeap.AllocateDescriptor();
        _resourceDescriptorHeap.FillDescriptor(_d3dDevice, descriptor, resource, srvDesc);

        return AssembleHandle<TypedBufferView>(uint32_t(descriptor), 0);
    }

    UniformBufferView DeviceD3D12::CreateUniformBufferView(const UniformBufferViewDesc& desc)
    {
        RN_ASSERT(desc.offsetInBytes % 256 == 0);
        RN_ASSERT(desc.sizeInBytes % 256 == 0);

        ID3D12Resource* resource = _buffers.GetHot(desc.buffer);
        D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {
            .BufferLocation = resource->GetGPUVirtualAddress() + desc.offsetInBytes,
            .SizeInBytes = desc.sizeInBytes,
        };

        ResourceDescriptor descriptor = _resourceDescriptorHeap.AllocateDescriptor();
        _resourceDescriptorHeap.FillDescriptor(_d3dDevice, descriptor, cbvDesc);

        return AssembleHandle<UniformBufferView>(uint32_t(descriptor), 0);
    }

    RWTexture2DView DeviceD3D12::CreateRWTexture2DView(const RWTexture2DViewDesc& desc)
    {
        D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {
            .Format = ToDXGIFormat(desc.format),
            .ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D,
            .Texture2D = {
                .MipSlice = desc.mipIndex,
                .PlaneSlice = 0,
            }
        };

        ID3D12Resource* resource = _texture2Ds.GetHot(desc.texture);
        ResourceDescriptor descriptor = _resourceDescriptorHeap.AllocateDescriptor();
        _resourceDescriptorHeap.FillDescriptor(_d3dDevice, descriptor, resource, uavDesc);

        return AssembleHandle<RWTexture2DView>(uint32_t(descriptor), 0);
    }

    RWTexture3DView DeviceD3D12::CreateRWTexture3DView(const RWTexture3DViewDesc& desc)
    {
        D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {
            .Format = ToDXGIFormat(desc.format),
            .ViewDimension = D3D12_UAV_DIMENSION_TEXTURE3D,
            .Texture3D = {
                .MipSlice = desc.mipIndex,
                .FirstWSlice = 0,
                .WSize = 0xFFFFFFFF
            }
        };

        ID3D12Resource* resource = _texture3Ds.GetHot(desc.texture);
        ResourceDescriptor descriptor = _resourceDescriptorHeap.AllocateDescriptor();
        _resourceDescriptorHeap.FillDescriptor(_d3dDevice, descriptor, resource, uavDesc);

        return AssembleHandle<RWTexture3DView>(uint32_t(descriptor), 0);
    }

    RWTexture2DArrayView DeviceD3D12::CreateRWTexture2DArrayView(const RWTexture2DArrayViewDesc& desc)
    {
        D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {
            .Format = ToDXGIFormat(desc.format),
            .ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2DARRAY,
            .Texture2DArray = {
                .MipSlice = desc.mipIndex,
                .FirstArraySlice = 0,
                .ArraySize = desc.arraySize,
                .PlaneSlice = 0,
            }
        };

        ID3D12Resource* resource = _texture2Ds.GetHot(desc.texture);
        ResourceDescriptor descriptor = _resourceDescriptorHeap.AllocateDescriptor();
        _resourceDescriptorHeap.FillDescriptor(_d3dDevice, descriptor, resource, uavDesc);

        return AssembleHandle<RWTexture2DArrayView>(uint32_t(descriptor), 0);
    }

    RWBufferView DeviceD3D12::CreateRWBufferView(const RWBufferViewDesc& desc)
    {
        RN_ASSERT((desc.offsetInBytes % 4) == 0);
        RN_ASSERT((desc.sizeInBytes % 4) == 0);

        D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {
            .Format = DXGI_FORMAT_R32_TYPELESS,
            .ViewDimension = D3D12_UAV_DIMENSION_BUFFER,
            .Buffer = {
                .FirstElement = desc.offsetInBytes / 4,
                .NumElements = desc.sizeInBytes / 4,
                .Flags = D3D12_BUFFER_UAV_FLAG_RAW
            }
        };

        ID3D12Resource* resource = _buffers.GetHot(desc.buffer);
        ResourceDescriptor descriptor = _resourceDescriptorHeap.AllocateDescriptor();
        _resourceDescriptorHeap.FillDescriptor(_d3dDevice, descriptor, resource, uavDesc);

        return AssembleHandle<RWBufferView>(uint32_t(descriptor), 0);
    }

    TLASView DeviceD3D12::CreateTLASView(const ASViewDesc& desc)
    {
        ID3D12Resource* tlasBuffer = _buffers.GetHot(desc.asBuffer);
        D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {
            .Format = DXGI_FORMAT_UNKNOWN,
            .ViewDimension = D3D12_SRV_DIMENSION_RAYTRACING_ACCELERATION_STRUCTURE,
            .Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING,
            .RaytracingAccelerationStructure = {
                .Location = tlasBuffer->GetGPUVirtualAddress() + desc.offsetInBytes,
            }
        };

        ResourceDescriptor descriptor = _resourceDescriptorHeap.AllocateDescriptor();
        _resourceDescriptorHeap.FillDescriptor(_d3dDevice, descriptor, nullptr, srvDesc);

        return AssembleHandle<TLASView>(uint32_t(descriptor), 0);
    }

    BLASView DeviceD3D12::CreateBLASView(const ASViewDesc& desc)
    {
        BLASData blasData = {};
        return _blases.Store(std::move(blasData));
    }

    void DeviceD3D12::Destroy(Buffer buffer)
    {
        QueueFrameFinalizerAction([](DeviceD3D12* device, void* data)
            {
                Buffer b = Buffer(ptrdiff_t(data));

                ID3D12Resource* resource = device->_buffers.GetHot(b);      
                if (resource)
                {
                    resource->Release();
                }

                device->_buffers.Remove(b);
            },
            reinterpret_cast<void*>(buffer));
    }

    void DeviceD3D12::Destroy(Texture2D texture)
    {
        QueueFrameFinalizerAction([](DeviceD3D12* device, void* data)
            {
                Texture2D t = Texture2D(ptrdiff_t(data));

                ID3D12Resource* resource = device->_texture2Ds.GetHot(t);      
                if (resource)
                {
                    resource->Release();
                }

                device->_texture2Ds.Remove(t);
            },
            reinterpret_cast<void*>(texture));
    }

    void DeviceD3D12::Destroy(Texture3D texture)
    {
        QueueFrameFinalizerAction([](DeviceD3D12* device, void* data)
            {
                Texture3D t = Texture3D(ptrdiff_t(data));

                ID3D12Resource* resource = device->_texture3Ds.GetHot(t);      
                if (resource)
                {
                    resource->Release();
                }

                device->_texture3Ds.Remove(t);
            },
            reinterpret_cast<void*>(texture));
    }

    void DeviceD3D12::Destroy(RenderTargetView view) 
    {
        QueueFrameFinalizerAction([](DeviceD3D12* device, void* data)
            {
                const D3D12_RENDER_TARGET_VIEW_DESC NULL_RTV = {
                    .Format = DXGI_FORMAT_R8G8B8A8_UNORM,
                    .ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D,
                };

                RenderTargetView view = RenderTargetView(ptrdiff_t(data));
                ResourceDescriptor descriptor = ResourceDescriptor(IndexFromHandle(view));
                device->_rtvDescriptorHeap.FillDescriptor(device->_d3dDevice, descriptor, nullptr, NULL_RTV);
                device->_rtvDescriptorHeap.FreeDescriptor(descriptor);
            },
            reinterpret_cast<void*>(view));
    }

    void DeviceD3D12::Destroy(DepthStencilView view) 
    {
        QueueFrameFinalizerAction([](DeviceD3D12* device, void* data)
            {
                const D3D12_DEPTH_STENCIL_VIEW_DESC NULL_DSV = {
                    .Format = DXGI_FORMAT_D32_FLOAT,
                    .ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D
                };

                DepthStencilView view = DepthStencilView(ptrdiff_t(data));
                ResourceDescriptor descriptor = ResourceDescriptor(IndexFromHandle(view));
                device->_dsvDescriptorHeap.FillDescriptor(device->_d3dDevice, descriptor, nullptr, NULL_DSV);
                device->_dsvDescriptorHeap.FreeDescriptor(descriptor);
            },
            reinterpret_cast<void*>(view));
    }

    void DeviceD3D12::Destroy(Texture2DView view) 
    {
        QueueFrameFinalizerAction([](DeviceD3D12* device, void* data)
            {
                const D3D12_SHADER_RESOURCE_VIEW_DESC NULL_SRV = {
                    .Format = DXGI_FORMAT_R8G8B8A8_UNORM,
                    .ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D,
                    .Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING,
                    .Texture2D = {
                        .MostDetailedMip = 0,
                        .MipLevels = 1,
                        .PlaneSlice = 0,
                        .ResourceMinLODClamp = 0.0f,
                    }
                };

                Texture2DView view = Texture2DView(ptrdiff_t(data));
                ResourceDescriptor descriptor = ResourceDescriptor(IndexFromHandle(view));
                device->_resourceDescriptorHeap.FillDescriptor(device->_d3dDevice, descriptor, nullptr, NULL_SRV);
                device->_resourceDescriptorHeap.FreeDescriptor(descriptor);
            },
            reinterpret_cast<void*>(view));
    }

    void DeviceD3D12::Destroy(Texture3DView view) 
    {
        QueueFrameFinalizerAction([](DeviceD3D12* device, void* data)
            {
                const D3D12_SHADER_RESOURCE_VIEW_DESC NULL_SRV = {
                    .Format = DXGI_FORMAT_R8G8B8A8_UNORM,
                    .ViewDimension = D3D12_SRV_DIMENSION_TEXTURE3D,
                    .Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING,
                    .Texture3D = {
                        .MostDetailedMip = 0,
                        .MipLevels = 1,
                        .ResourceMinLODClamp = 0.0f,
                    }
                };

                Texture3DView view = Texture3DView(ptrdiff_t(data));
                ResourceDescriptor descriptor = ResourceDescriptor(IndexFromHandle(view));
                device->_resourceDescriptorHeap.FillDescriptor(device->_d3dDevice, descriptor, nullptr, NULL_SRV);
                device->_resourceDescriptorHeap.FreeDescriptor(descriptor);
            },
            reinterpret_cast<void*>(view));
    }

    void DeviceD3D12::Destroy(TextureCubeView view) 
    {
        QueueFrameFinalizerAction([](DeviceD3D12* device, void* data)
            {
                const D3D12_SHADER_RESOURCE_VIEW_DESC NULL_SRV = {
                    .Format = DXGI_FORMAT_R8G8B8A8_UNORM,
                    .ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE,
                    .Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING,
                    .Texture3D = {
                        .MostDetailedMip = 0,
                        .MipLevels = 1,
                        .ResourceMinLODClamp = 0.0f,
                    }
                };

                TextureCubeView view = TextureCubeView(ptrdiff_t(data));
                ResourceDescriptor descriptor = ResourceDescriptor(IndexFromHandle(view));
                device->_resourceDescriptorHeap.FillDescriptor(device->_d3dDevice, descriptor, nullptr, NULL_SRV);
                device->_resourceDescriptorHeap.FreeDescriptor(descriptor);
            },
            reinterpret_cast<void*>(view));
    }

    void DeviceD3D12::Destroy(Texture2DArrayView view) 
    {
        QueueFrameFinalizerAction([](DeviceD3D12* device, void* data)
            {
                const D3D12_SHADER_RESOURCE_VIEW_DESC NULL_SRV = {
                    .Format = DXGI_FORMAT_R8G8B8A8_UNORM,
                    .ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DARRAY,
                    .Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING,
                    .Texture2DArray = {
                        .MostDetailedMip = 0,
                        .MipLevels = 1,
                        .FirstArraySlice = 0,
                        .ArraySize = 1,
                        .ResourceMinLODClamp = 0.0f,
                    }
                };

                Texture2DArrayView view = Texture2DArrayView(ptrdiff_t(data));
                ResourceDescriptor descriptor = ResourceDescriptor(IndexFromHandle(view));
                device->_resourceDescriptorHeap.FillDescriptor(device->_d3dDevice, descriptor, nullptr, NULL_SRV);
                device->_resourceDescriptorHeap.FreeDescriptor(descriptor);
            },
            reinterpret_cast<void*>(view));
    }

    void DeviceD3D12::Destroy(BufferView view) 
    {
        QueueFrameFinalizerAction([](DeviceD3D12* device, void* data)
            {
                const D3D12_SHADER_RESOURCE_VIEW_DESC NULL_SRV = {
                    .Format = DXGI_FORMAT_R32_TYPELESS,
                    .ViewDimension = D3D12_SRV_DIMENSION_BUFFER,
                    .Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING,
                    .Buffer = {
                        .FirstElement = 0,
                        .NumElements = 1,
                        .StructureByteStride = 0,
                        .Flags = D3D12_BUFFER_SRV_FLAG_RAW
                    }
                };

                BufferView view = BufferView(ptrdiff_t(data));
                ResourceDescriptor descriptor = ResourceDescriptor(IndexFromHandle(view));
                device->_resourceDescriptorHeap.FillDescriptor(device->_d3dDevice, descriptor, nullptr, NULL_SRV);
                device->_resourceDescriptorHeap.FreeDescriptor(descriptor);
            },
            reinterpret_cast<void*>(view));
    }

    void DeviceD3D12::Destroy(TypedBufferView view) 
    {
        QueueFrameFinalizerAction([](DeviceD3D12* device, void* data)
            {
                const D3D12_SHADER_RESOURCE_VIEW_DESC NULL_SRV = {
                    .Format = DXGI_FORMAT_R32_TYPELESS,
                    .ViewDimension = D3D12_SRV_DIMENSION_BUFFER,
                    .Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING,
                    .Buffer = {
                        .FirstElement = 0,
                        .NumElements = 1,
                        .StructureByteStride = 4,
                    }
                };

                TypedBufferView view = TypedBufferView(ptrdiff_t(data));
                ResourceDescriptor descriptor = ResourceDescriptor(IndexFromHandle(view));
                device->_resourceDescriptorHeap.FillDescriptor(device->_d3dDevice, descriptor, nullptr, NULL_SRV);
                device->_resourceDescriptorHeap.FreeDescriptor(descriptor);
            },
            reinterpret_cast<void*>(view));
    }

    void DeviceD3D12::Destroy(UniformBufferView view) 
    {
        QueueFrameFinalizerAction([](DeviceD3D12* device, void* data)
            {
                const D3D12_CONSTANT_BUFFER_VIEW_DESC NULL_CBV = {
                    .BufferLocation = 0,
                    .SizeInBytes = 256
                };

                UniformBufferView view = UniformBufferView(ptrdiff_t(data));
                ResourceDescriptor descriptor = ResourceDescriptor(IndexFromHandle(view));
                device->_resourceDescriptorHeap.FillDescriptor(device->_d3dDevice, descriptor, NULL_CBV);
                device->_resourceDescriptorHeap.FreeDescriptor(descriptor);
            },
            reinterpret_cast<void*>(view));
    }

    void DeviceD3D12::Destroy(RWTexture2DView view) 
    {
        QueueFrameFinalizerAction([](DeviceD3D12* device, void* data)
            {
                const D3D12_UNORDERED_ACCESS_VIEW_DESC NULL_UAV = {
                    .Format = DXGI_FORMAT_R8G8B8A8_UNORM,
                    .ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D
                };

                RWTexture2DView view = RWTexture2DView(ptrdiff_t(data));
                ResourceDescriptor descriptor = ResourceDescriptor(IndexFromHandle(view));
                device->_resourceDescriptorHeap.FillDescriptor(device->_d3dDevice, descriptor, nullptr, NULL_UAV);
                device->_resourceDescriptorHeap.FreeDescriptor(descriptor);
            },
            reinterpret_cast<void*>(view));
    }

    void DeviceD3D12::Destroy(RWTexture3DView view) 
    {
        QueueFrameFinalizerAction([](DeviceD3D12* device, void* data)
            {
                const D3D12_UNORDERED_ACCESS_VIEW_DESC NULL_UAV = {
                    .Format = DXGI_FORMAT_R8G8B8A8_UNORM,
                    .ViewDimension = D3D12_UAV_DIMENSION_TEXTURE3D,
                    .Texture3D = {
                        .WSize = 0xFFFFFFFF
                    }
                };

                RWTexture3DView view = RWTexture3DView(ptrdiff_t(data));
                ResourceDescriptor descriptor = ResourceDescriptor(IndexFromHandle(view));
                device->_resourceDescriptorHeap.FillDescriptor(device->_d3dDevice, descriptor, nullptr, NULL_UAV);
                device->_resourceDescriptorHeap.FreeDescriptor(descriptor);
            },
            reinterpret_cast<void*>(view));
    }

    void DeviceD3D12::Destroy(RWTexture2DArrayView view) 
    {
         QueueFrameFinalizerAction([](DeviceD3D12* device, void* data)
            {
                const D3D12_UNORDERED_ACCESS_VIEW_DESC NULL_UAV = {
                    .Format = DXGI_FORMAT_R8G8B8A8_UNORM,
                    .ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2DARRAY,
                    .Texture2DArray = {
                        .ArraySize = 1
                    }
                };

                RWTexture2DArrayView view = RWTexture2DArrayView(ptrdiff_t(data));
                ResourceDescriptor descriptor = ResourceDescriptor(IndexFromHandle(view));
                device->_resourceDescriptorHeap.FillDescriptor(device->_d3dDevice, descriptor, nullptr, NULL_UAV);
                device->_resourceDescriptorHeap.FreeDescriptor(descriptor);
            },
            reinterpret_cast<void*>(view));
    }

    void DeviceD3D12::Destroy(RWBufferView view) 
    {
        QueueFrameFinalizerAction([](DeviceD3D12* device, void* data)
            {
                const D3D12_UNORDERED_ACCESS_VIEW_DESC NULL_UAV = {
                    .Format = DXGI_FORMAT_R32_TYPELESS,
                    .ViewDimension = D3D12_UAV_DIMENSION_BUFFER,
                    .Buffer = {
                        .NumElements = 1,
                        .Flags = D3D12_BUFFER_UAV_FLAG_RAW
                    }
                };

                RWBufferView view = RWBufferView(ptrdiff_t(data));
                ResourceDescriptor descriptor = ResourceDescriptor(IndexFromHandle(view));
                device->_resourceDescriptorHeap.FillDescriptor(device->_d3dDevice, descriptor, nullptr, NULL_UAV);
                device->_resourceDescriptorHeap.FreeDescriptor(descriptor);
            },
            reinterpret_cast<void*>(view));
    }

    void DeviceD3D12::Destroy(TLASView view) 
    {
        QueueFrameFinalizerAction([](DeviceD3D12* device, void* data)
            {
                const D3D12_SHADER_RESOURCE_VIEW_DESC NULL_SRV = {
                    .Format = DXGI_FORMAT_UNKNOWN,
                    .ViewDimension = D3D12_SRV_DIMENSION_RAYTRACING_ACCELERATION_STRUCTURE,
                    .Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING,
                };

                TLASView view = TLASView(ptrdiff_t(data));
                ResourceDescriptor descriptor = ResourceDescriptor(IndexFromHandle(view));
                device->_resourceDescriptorHeap.FillDescriptor(device->_d3dDevice, descriptor, nullptr, NULL_SRV);
                device->_resourceDescriptorHeap.FreeDescriptor(descriptor);
            },
            reinterpret_cast<void*>(view));
    }

    void DeviceD3D12::Destroy(BLASView view) 
    {
        _blases.Remove(view);
    }

    ResourceFootprint DeviceD3D12::CalculateTextureFootprint(const Texture2DDesc& desc)
    {
        D3D12_RESOURCE_FLAGS resourceFlags = ToD3D12ResourceFlags(desc.flags);
        DXGI_FORMAT dxgiFormat = ToDXGIFormat(desc.format);
        if ((resourceFlags & D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL))
        {
            if (!(resourceFlags & D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE))
            {
                dxgiFormat = ToTypelessDXGIFormat(desc.format);
            }
            else
            {
                dxgiFormat = ToDXGIFormat(ToDepthEquivalent(desc.format));
            }
        }

        D3D12_RESOURCE_DESC1 resourceDesc = {    
            .Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D,
            .Width = desc.width,
            .Height = desc.height,
            .DepthOrArraySize = UINT16(desc.arraySize),
            .MipLevels = UINT16(desc.mipLevels),
            .Format = dxgiFormat,
            .SampleDesc = { 1, 0 },
            .Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN,
            .Flags = resourceFlags,
        };

        D3D12_RESOURCE_ALLOCATION_INFO1 allocationInfo = {};
        _d3dDevice->GetResourceAllocationInfo2(0, 1, &resourceDesc, &allocationInfo);

        return {
            .sizeInBytes = allocationInfo.SizeInBytes,
            .alignment = allocationInfo.Alignment
        };
    }

    ResourceFootprint DeviceD3D12::CalculateTextureFootprint(const Texture3DDesc& desc)
    {
        D3D12_RESOURCE_DESC1 resourceDesc = {
            .Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE3D,
            .Width = desc.width,
            .Height = desc.height,
            .DepthOrArraySize = UINT16(desc.depth),
            .MipLevels = UINT16(desc.mipLevels),
            .Format = ToDXGIFormat(desc.format),
            .SampleDesc = { 1, 0 },
            .Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN,
            .Flags = ToD3D12ResourceFlags(desc.flags),
        };

        D3D12_RESOURCE_ALLOCATION_INFO1 allocationInfo = {};
        _d3dDevice->GetResourceAllocationInfo2(0, 1, &resourceDesc, &allocationInfo);

        return {
            .sizeInBytes = allocationInfo.SizeInBytes,
            .alignment = allocationInfo.Alignment
        };
    }

    ASFootprint DeviceD3D12::CalculateBLASFootprint(Span<const BLASTriangleGeometryDesc> geometryDescs)
    {
        MemoryScope SCOPE;
        ScopedVector<D3D12_RAYTRACING_GEOMETRY_DESC> srcD3DGeoDesc;
        srcD3DGeoDesc.reserve(geometryDescs.size());

        for (const BLASTriangleGeometryDesc& geoDesc : geometryDescs)
        {
            ID3D12Resource* indexBuffer = _buffers.GetHot(geoDesc.indexBuffer);
            ID3D12Resource* vertexBuffer = _buffers.GetHot(geoDesc.vertexBuffer);

            D3D12_RAYTRACING_GEOMETRY_DESC rtGeoDesc = {
                .Type = D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES,
                .Flags = D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE,
                .Triangles = {
                    .IndexFormat = geoDesc.indexFormat == IndexFormat::Uint16 ? DXGI_FORMAT_R16_UINT : DXGI_FORMAT_R32_UINT,
                    .VertexFormat = DXGI_FORMAT_R32G32B32_FLOAT,
                    .IndexCount = geoDesc.indexCount,
                    .VertexCount = geoDesc.vertexCount,
                    .IndexBuffer = indexBuffer->GetGPUVirtualAddress() + geoDesc.offsetInIndexBuffer,
                    .VertexBuffer = {
                        .StartAddress = vertexBuffer->GetGPUVirtualAddress() + geoDesc.offsetInVertexBuffer,
                        .StrideInBytes = geoDesc.vertexStride,
                    }
                },
            };
            srcD3DGeoDesc.emplace_back(rtGeoDesc);
        }

        D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS inputs = {
            .Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL,
            .Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE,
            .NumDescs = UINT(geometryDescs.size()),
            .DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY,
            .pGeometryDescs = srcD3DGeoDesc.data(),
        };

        D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO prebuildInfo = {};
        _d3dDevice->GetRaytracingAccelerationStructurePrebuildInfo(&inputs, &prebuildInfo);

        return {
            .destDataSizeInBytes = prebuildInfo.ResultDataMaxSizeInBytes,
            .scratchDataSizeInBytes = prebuildInfo.ScratchDataSizeInBytes,
            .updateDataSizeInBytes = prebuildInfo.UpdateScratchDataSizeInBytes,
        };
    }

    ASFootprint DeviceD3D12::CalculateTLASFootprint(Buffer instanceBuffer, uint32_t offsetInInstanceBuffer, uint32_t instanceCount)
    {
        ID3D12Resource* bufferResource = _buffers.GetHot(instanceBuffer);

        D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS inputs = {
            .Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL,
            .Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE,
            .NumDescs = instanceCount,
            .DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY,
            .InstanceDescs = bufferResource->GetGPUVirtualAddress() + offsetInInstanceBuffer,
        };

        D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO prebuildInfo = {};
        _d3dDevice->GetRaytracingAccelerationStructurePrebuildInfo(&inputs, &prebuildInfo);

        return {
            .destDataSizeInBytes = prebuildInfo.ResultDataMaxSizeInBytes,
            .scratchDataSizeInBytes = prebuildInfo.ScratchDataSizeInBytes,
            .updateDataSizeInBytes = prebuildInfo.UpdateScratchDataSizeInBytes,
        };
    }

    SRTFootprint DeviceD3D12::CalculateShaderRecordTableFootprint(const SRTFootprintDesc& desc)
    {
        SRTFootprint footprint{};

        // NOTE: This code needs to be updated if we ever end up allowing more data in the RT-local root signature!
        const uint32_t localRootSigDataSize = sizeof(D3D12_GPU_VIRTUAL_ADDRESS);    // Per-hit root constant buffer
        const uint32_t shaderRecordSize = uint32_t(AlignSize(D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES + localRootSigDataSize, D3D12_RAYTRACING_SHADER_RECORD_BYTE_ALIGNMENT));

        uint32_t currentSize = 0;

        // Ray gen record
        if (TestFlag(desc.flags, ShaderRecordTableFlags::ContainsRayGenShader))
        {
            footprint.rayGenRecordOffset = currentSize;
            footprint.rayGenRecordSize = shaderRecordSize;
            currentSize += footprint.rayGenRecordSize;
        }

        // Miss shader table
        if (desc.missShaderCount > 0)
        {
            currentSize = uint32_t(AlignSize(currentSize, D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT));
            footprint.missRecordOffset = currentSize;
            footprint.missRecordBufferSize = desc.missShaderCount * shaderRecordSize;
            footprint.missRecordStride = shaderRecordSize;
            currentSize += footprint.missRecordBufferSize;
        }

        // Hit group table
        if (desc.hitGroupCount > 0)
        {
            currentSize = uint32_t(AlignSize(currentSize, D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT));
            footprint.hitGroupRecordOffset = currentSize;
            footprint.hitGroupRecordBufferSize = desc.hitGroupCount * shaderRecordSize;
            footprint.hitGroupRecordStride = shaderRecordSize;
            currentSize += footprint.hitGroupRecordBufferSize;
        }

        footprint.bufferSize = currentSize;
        return footprint;
    }

    namespace
    {
        uint64_t CalculateGenericMipUploadDescs(
            ID3D12Device10* device,
            const D3D12_RESOURCE_DESC1& resourceDesc,
            Span<MipUploadDesc> outMipUploadDescs)
        {
            constexpr uint32_t BATCH_SIZE = 16;
            D3D12_PLACED_SUBRESOURCE_FOOTPRINT footprints[BATCH_SIZE] = {};
            UINT numRows[BATCH_SIZE] = {};
            UINT64 rowSizeInBytes[BATCH_SIZE] = {};
            UINT64 totalBytes = 0;

            uint32_t mipLevelsToProcess = resourceDesc.MipLevels;
            uint32_t baseMipIndex = 0;

            while (mipLevelsToProcess > 0)
            {
                const uint32_t batchSize = std::min(BATCH_SIZE, mipLevelsToProcess);

                device->GetCopyableFootprints1(
                    &resourceDesc,
                    baseMipIndex,
                    batchSize,
                    0ull,
                    footprints,
                    numRows,
                    rowSizeInBytes,
                    &totalBytes);

                for (uint32_t i = 0; i < batchSize; ++i)
                {
                    MipUploadDesc& mipUploadDesc = outMipUploadDescs[baseMipIndex + i];

                    mipUploadDesc = {
                        .offsetInUploadBuffer = footprints[i].Offset,
                        .format = DXGIToTextureFormat(footprints[i].Footprint.Format),
                        .width = footprints[i].Footprint.Width,
                        .height = footprints[i].Footprint.Height,
                        .depth = footprints[i].Footprint.Depth,
                        .rowPitch = footprints[i].Footprint.RowPitch,
                        .rowCount = numRows[i],
                        .rowSizeInBytes = rowSizeInBytes[i],
                        .totalSizeInBytes = footprints[i].Footprint.RowPitch * numRows[i],
                    };
                }

                mipLevelsToProcess -= batchSize;
                baseMipIndex += batchSize;
            }

            return totalBytes;
        }
    }

    uint64_t DeviceD3D12::CalculateMipUploadDescs(const Texture2DDesc& desc, Span<MipUploadDesc> outMipUploadDescs)
    {
        D3D12_RESOURCE_FLAGS resourceFlags = ToD3D12ResourceFlags(desc.flags);
        DXGI_FORMAT dxgiFormat = ToDXGIFormat(desc.format);
        if ((resourceFlags & D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL))
        {
            if (!(resourceFlags & D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE))
            {
                dxgiFormat = ToTypelessDXGIFormat(desc.format);
            }
            else
            {
                dxgiFormat = ToDXGIFormat(ToDepthEquivalent(desc.format));
            }
        }

        D3D12_RESOURCE_DESC1 resourceDesc = {    
            .Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D,
            .Width = desc.width,
            .Height = desc.height,
            .DepthOrArraySize = UINT16(desc.arraySize),
            .MipLevels = UINT16(desc.mipLevels),
            .Format = dxgiFormat,
            .SampleDesc = { 1, 0 },
            .Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN,
            .Flags = resourceFlags,
        };

        return CalculateGenericMipUploadDescs(_d3dDevice, resourceDesc, outMipUploadDescs);
    }

    uint64_t DeviceD3D12::CalculateMipUploadDescs(const Texture3DDesc& desc, Span<MipUploadDesc> outMipUploadDescs)
    {
        D3D12_RESOURCE_DESC1 resourceDesc = {
            .Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE3D,
            .Width = desc.width,
            .Height = desc.height,
            .DepthOrArraySize = UINT16(desc.depth),
            .MipLevels = UINT16(desc.mipLevels),
            .Format = ToDXGIFormat(desc.format),
            .SampleDesc = { 1, 0 },
            .Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN,
            .Flags = ToD3D12ResourceFlags(desc.flags),
        };

        return CalculateGenericMipUploadDescs(_d3dDevice, resourceDesc, outMipUploadDescs);
    }

    ResourceFootprint DeviceD3D12::CalculateTLASInstanceBufferFootprint(uint32_t instanceCount)
    {
        return {
            .sizeInBytes = instanceCount * sizeof(D3D12_RAYTRACING_INSTANCE_DESC),
            .alignment = sizeof(D3D12_RAYTRACING_INSTANCE_DESC)
        };
    }

    void DeviceD3D12::PopulateTLASInstances(Span<const TLASInstanceDesc> instances, Span<unsigned char*> destData)
    {
        RN_ASSERT(destData.size() >= instances.size() * sizeof(D3D12_RAYTRACING_INSTANCE_DESC));
        constexpr uint32_t MAX_INSTANCE_BATCH_SIZE = 32;
        D3D12_RAYTRACING_INSTANCE_DESC instanceBatch[MAX_INSTANCE_BATCH_SIZE] = {};

        uint32_t instanceIdx = 0;
        D3D12_RAYTRACING_INSTANCE_DESC* destInstances = reinterpret_cast<D3D12_RAYTRACING_INSTANCE_DESC*>(destData.data());
        for (const TLASInstanceDesc& instance: instances)
        {
            ID3D12Resource* blasResource = _buffers.GetHot(instance.blasBuffer);

            destInstances[instanceIdx] = {
                .InstanceID = instance.instanceID,
                .InstanceMask = instance.instanceMask,
                .InstanceContributionToHitGroupIndex = instance.hitGroupModifier,
                .Flags = 0,
                .AccelerationStructure = blasResource->GetGPUVirtualAddress() + instance.offsetInBlasBuffer,
            };

            for (uint32_t row = 0; row < 3; ++row)
            {
                for (uint32_t col = 0; col < 4; ++col)
                {
                     destInstances[instanceIdx].Transform[row][col] = instance.transform[col][row];
                }
            }

            ++instanceIdx;
        }
    }

    TemporaryResourceAllocator* DeviceD3D12::GetTemporaryResourceAllocatorForCurrentThread()
    {
        std::unique_lock lock(_uploadAllocatorMutex);
        auto it = _uploadAllocators.find(std::this_thread::get_id());
        if (it == _uploadAllocators.end())
        {
            TemporaryResourceAllocator* newAllocator = TrackedNew<TemporaryResourceAllocator>(MemoryCategory::RHI,
                this, 
                MAX_FRAME_LATENCY, 
                GPUAllocationFlags::HostUpload,
                BufferCreationFlags::AllowShaderReadOnly | BufferCreationFlags::AllowUniformBuffer,
                [](Device* device, Buffer buffer, uint64_t offset, uint64_t size) -> void* { return reinterpret_cast<DeviceD3D12*>(device)->MapBuffer(buffer, offset, size); },
                [](Device* device, Buffer buffer, uint64_t offset, uint64_t size) { reinterpret_cast<DeviceD3D12*>(device)->UnmapBuffer(buffer, offset, size); });

            it = _uploadAllocators.insert_or_assign(std::this_thread::get_id(), newAllocator).first;
        }

        return it->second;
    }

    TemporaryResourceAllocator* DeviceD3D12::GetReadbackAllocatorForCurrentThread()
    {
        std::unique_lock lock(_readbackAllocatorMutex);
        auto it = _readbackAllocators.find(std::this_thread::get_id());
        if (it == _readbackAllocators.end())
        {
            TemporaryResourceAllocator* newAllocator = TrackedNew<TemporaryResourceAllocator>(MemoryCategory::RHI,
                this, 
                MAX_FRAME_LATENCY, 
                GPUAllocationFlags::HostReadback,
                BufferCreationFlags::None,
                [](Device* device, Buffer buffer, uint64_t offset, uint64_t size) -> void* { return reinterpret_cast<DeviceD3D12*>(device)->MapBuffer(buffer, offset, size); },
                [](Device* device, Buffer buffer, uint64_t offset, uint64_t size) { reinterpret_cast<DeviceD3D12*>(device)->UnmapBuffer(buffer, offset, 0); });

            it = _readbackAllocators.insert_or_assign(std::this_thread::get_id(), newAllocator).first;
        }

        return it->second;
    }

    ID3D12PipelineState* DeviceD3D12::Resolve(RasterPipeline pipeline) const
    {
        return _rasterPipelines.GetHot(pipeline);
    }

    ID3D12PipelineState* DeviceD3D12::Resolve(ComputePipeline pipeline) const
    {
        return _computePipelines.GetHot(pipeline);
    }

    ID3D12StateObject* DeviceD3D12::Resolve(RTPipeline pipeline) const
    {
        return _rtPipelines.GetHot(pipeline);
    }

    RasterPipelineData DeviceD3D12::ResolveAdditionalData(RasterPipeline pipeline) const
    {
        return _rasterPipelines.GetCold(pipeline);
    }


    ID3D12Resource* DeviceD3D12::Resolve(Buffer buffer) const
    {
        return _buffers.GetHot(buffer);
    }

    ID3D12Resource* DeviceD3D12::Resolve(Texture2D texture) const
    {
        return _texture2Ds.GetHot(texture);
    }

    ID3D12Resource* DeviceD3D12::Resolve(Texture3D texture) const
    {
        return _texture3Ds.GetHot(texture);
    }

    D3D12_CPU_DESCRIPTOR_HANDLE DeviceD3D12::Resolve(RenderTargetView view) const
    {
        return _rtvDescriptorHeap.Resolve(ResourceDescriptor(IndexFromHandle(view)));
    }

    D3D12_CPU_DESCRIPTOR_HANDLE DeviceD3D12::Resolve(DepthStencilView view) const
    {
        return _dsvDescriptorHeap.Resolve(ResourceDescriptor(IndexFromHandle(view)));
    }

    Texture2D DeviceD3D12::PlaceTexture2D(ID3D12Resource* resource)
    {
        return _texture2Ds.Store(std::move(resource));
    }


}