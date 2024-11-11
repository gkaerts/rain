#pragma once

#include "common/common.hpp"
#include "common/memory/index_allocator.hpp"

#include "d3d12.h"

namespace rn::rhi
{
    enum class ResourceDescriptor : uint32_t
    {
        Invalid = 0xFFFFFFFF
    };

    class DescriptorHeap
    {
    public:

        DescriptorHeap();
        ~DescriptorHeap();

        DescriptorHeap(const DescriptorHeap&) = delete;
        DescriptorHeap& operator=(const DescriptorHeap&) = delete;

        DescriptorHeap(DescriptorHeap&&) = default;
        DescriptorHeap& operator=(DescriptorHeap&&) = default;

        void InitAsResourceHeap(ID3D12Device10* device);
        void InitAsSamplerHeap(ID3D12Device10* device);
        void InitAsRTVHeap(ID3D12Device10* device);
        void InitAsDSVHeap(ID3D12Device10* device);

        ResourceDescriptor AllocateDescriptor();
        void FreeDescriptor(ResourceDescriptor descriptor);

        void FillDescriptor(ID3D12Device10* device, ResourceDescriptor descriptor, ID3D12Resource* resource, const D3D12_SHADER_RESOURCE_VIEW_DESC& desc);
        void FillDescriptor(ID3D12Device10* device, ResourceDescriptor descriptor, ID3D12Resource* resource, const D3D12_UNORDERED_ACCESS_VIEW_DESC& desc);
        void FillDescriptor(ID3D12Device10* device, ResourceDescriptor descriptor, const D3D12_CONSTANT_BUFFER_VIEW_DESC& desc);
        void FillDescriptor(ID3D12Device10* device, ResourceDescriptor descriptor, ID3D12Resource* resource, const D3D12_RENDER_TARGET_VIEW_DESC& desc);
        void FillDescriptor(ID3D12Device10* device, ResourceDescriptor descriptor, ID3D12Resource* resource, const D3D12_DEPTH_STENCIL_VIEW_DESC& desc);
        void FillDescriptor(ID3D12Device10* device, ResourceDescriptor descriptor, const D3D12_SAMPLER_DESC& desc);

        D3D12_CPU_DESCRIPTOR_HANDLE Resolve(ResourceDescriptor descriptor) const;

    private:

        enum class Type
        {
            Resource,
            Sampler,
            RTV,
            DSV,
            Unknown
        };

        Type _type = Type::Unknown;
        ID3D12DescriptorHeap* _heap = nullptr;
        uint32_t _descriptorSizeInBytes = 0;

        IndexAllocator _indexAllocator;
    };
}