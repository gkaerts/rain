#define NOMINMAX
#include "device_d3d12.hpp"
#include "pipeline_d3d12.hpp"
#include "swap_chain_d3d12.hpp"
#include "rhi/rhi_d3d12.hpp"

#include "common/memory/memory.hpp"
#include "common/log/log.hpp"

#include "d3d12.h"
#include <dxgi1_6.h>
#include <dxgidebug.h>

extern "C" { __declspec(dllexport) extern const UINT D3D12SDKVersion = 614;}
extern "C" { __declspec(dllexport) extern const char* D3D12SDKPath = ".\\"; }
namespace rn::rhi
{
    constexpr const D3D_FEATURE_LEVEL DEFAULT_D3D_FEATURE_LEVEL = D3D_FEATURE_LEVEL_12_1;

    RN_DEFINE_MEMORY_CATEGORY(D3D12);
    RN_DEFINE_LOG_CATEGORY(D3D12);

    Device* CreateD3D12Device(const DeviceD3D12Options& options, const DeviceMemorySettings& memorySettings)
    {
        return TrackedNew<DeviceD3D12>(MemoryCategory::D3D12, options, memorySettings);
    }

    void FinalizerQueue::Queue(FnOnFinalize fn, void* data)
    {
        int32_t actionIdx = _highWatermark.fetch_add(1);
        FinalizerAction action = {
            .onFinalize = fn,
            .data = data
        };

        if (actionIdx < MAX_FAST_FINALIZER_ACTIONS)
        {
            _actions[actionIdx] = action;
        }
        else
        {
            std::unique_lock lock(_overflowMutex);
            _overflowActions.emplace_back(action);
        }
    }

    void FinalizerQueue::Flush(DeviceD3D12* device)
    {
        int32_t actionCount = std::min(int32_t(MAX_FAST_FINALIZER_ACTIONS), _highWatermark.load());
        for (int32_t i = 0; i < actionCount; ++i)
        {
            _actions[i].onFinalize(device, _actions[i].data);
            _actions[i] = {};
        }
        _highWatermark = 0;

        for (FinalizerAction& a : _overflowActions)
        {
            a.onFinalize(device, a.data);
            a = {};
        }
        _overflowActions.clear();
    }

    namespace
    {
        ID3D12Debug3* EnableD3D12DebugLayer(bool enableGPUBasedValidation)
        {
            LogInfo(LogCategory::D3D12, "Enabling D3D12 debug layer");

            ID3D12Debug3* d3dDebug = nullptr;
            HRESULT hr = D3D12GetDebugInterface(__uuidof(ID3D12Debug3), (void**)&d3dDebug);
            if (FAILED(hr))
            {
                LogError(LogCategory::D3D12, "Failed to retrieve ID3D12Debug3 interface");
                return nullptr;
            }

            d3dDebug->EnableDebugLayer();

            if (enableGPUBasedValidation)
            {
                LogInfo(LogCategory::D3D12, "Enabling D3D12 GPU-based validation");
                d3dDebug->SetEnableGPUBasedValidation(TRUE);
            }

            return d3dDebug;
        }

        IDXGIFactory7* CreateDXGIFactory()
        {
            IDXGIFactory7* dxgiFactory = nullptr;

            HRESULT hr = CreateDXGIFactory2(0,
                __uuidof(IDXGIFactory7),
                (void**)(&dxgiFactory));

            RN_ASSERT(SUCCEEDED(hr));
            return dxgiFactory;
        }

        IDXGIAdapter4* GetDXGIAdapterAtIndex(IDXGIFactory7* dxgiFactory, uint32_t adapterIndex)
        {
            IDXGIAdapter1* adapter1 = nullptr;
            IDXGIAdapter4* adapter4 = nullptr;

            HRESULT hr = dxgiFactory->EnumAdapters1(
                adapterIndex,
                &adapter1);
            
            if (FAILED(hr))
            {
                LogError(LogCategory::D3D12, "Failed to enumate DXGI adapter at index {}", adapterIndex);
                if (adapterIndex != 0)
                {
                    LogInfo(LogCategory::D3D12, "Retrying adapter enumeration with default adapter");
                    hr = dxgiFactory->EnumAdapters1(
                        0,
                        &adapter1);

                    RN_ASSERT(SUCCEEDED(hr));
                }
            }

            hr = adapter1->QueryInterface<IDXGIAdapter4>(&adapter4);
            RN_ASSERT(SUCCEEDED(hr));

            adapter1->Release();

            return adapter4;  
        }
    
        ID3D12Device10* CreateD3D12Device(IDXGIAdapter4* adapter)
        {
            ID3D12Device10* d3dDevice = nullptr;
            HRESULT hr = D3D12CreateDevice(adapter, DEFAULT_D3D_FEATURE_LEVEL, __uuidof(ID3D12Device10), (void**)(&d3dDevice));
            RN_ASSERT(SUCCEEDED(hr));

            return d3dDevice;
        }

        DeviceCaps EnumerateDeviceCaps(ID3D12Device10* d3dDevice)
        {
            // Architecture
            D3D12_FEATURE_DATA_ARCHITECTURE1 featureDataArchitecture{};
            RN_ASSERT(SUCCEEDED(
                d3dDevice->CheckFeatureSupport(
                    D3D12_FEATURE_ARCHITECTURE1,
                    &featureDataArchitecture,
                    sizeof(featureDataArchitecture))));

            // Ray tracing
            D3D12_FEATURE_DATA_D3D12_OPTIONS5 options5{};
            RN_ASSERT(SUCCEEDED(
                d3dDevice->CheckFeatureSupport(
                    D3D12_FEATURE_D3D12_OPTIONS5,
                    &options5,
                    sizeof(options5))));

            // VRS
            D3D12_FEATURE_DATA_D3D12_OPTIONS6 options6{};
            RN_ASSERT(SUCCEEDED(
                d3dDevice->CheckFeatureSupport(
                    D3D12_FEATURE_D3D12_OPTIONS6,
                    &options6,
                    sizeof(options6))));

            // Mesh shaders
            D3D12_FEATURE_DATA_D3D12_OPTIONS7 options7{};
            RN_ASSERT(SUCCEEDED(
                d3dDevice->CheckFeatureSupport(
                    D3D12_FEATURE_D3D12_OPTIONS7,
                    &options7,
                    sizeof(options7))));

            return
            {
                .hasHardwareRaytracing = options5.RaytracingTier >= D3D12_RAYTRACING_TIER_1_1,
                .hasVariableRateShading = options6.VariableShadingRateTier >= D3D12_VARIABLE_SHADING_RATE_TIER_1,
                .hasMeshShaders = options7.MeshShaderTier >= D3D12_MESH_SHADER_TIER_1,
                .isUMA = featureDataArchitecture.CacheCoherentUMA != 0,
                .hasMemorylessAllocations = false
            };
        }

        ID3D12CommandQueue* CreateGraphicsQueue(ID3D12Device10* device)
        {
            D3D12_COMMAND_QUEUE_DESC desc = {
                .Type = D3D12_COMMAND_LIST_TYPE_DIRECT,
            };

            ID3D12CommandQueue* queue = nullptr;
            RN_ASSERT(SUCCEEDED(device->CreateCommandQueue(&desc, __uuidof(ID3D12CommandQueue), (void**)&queue)));

            return queue;
        }

        ID3D12Fence* CreateFence(ID3D12Device10* device)
        {
            ID3D12Fence* fence = nullptr;
            RN_ASSERT(SUCCEEDED(device->CreateFence(0, D3D12_FENCE_FLAG_NONE, __uuidof(ID3D12Fence), (void**)(&fence))));

            return fence;
        }

        constexpr const D3D12_INDIRECT_ARGUMENT_DESC COMMAND_SIGNATURE_ARGUMENTS[] =
        {
            { .Type = D3D12_INDIRECT_ARGUMENT_TYPE_DRAW },
            { .Type = D3D12_INDIRECT_ARGUMENT_TYPE_DRAW_INDEXED },
            { .Type = D3D12_INDIRECT_ARGUMENT_TYPE_DISPATCH },
            { .Type = D3D12_INDIRECT_ARGUMENT_TYPE_DISPATCH_MESH },
            { .Type = D3D12_INDIRECT_ARGUMENT_TYPE_DISPATCH_RAYS },
        };
        RN_MATCH_ENUM_AND_ARRAY(COMMAND_SIGNATURE_ARGUMENTS, CommandSignatureType);

        constexpr const D3D12_COMMAND_SIGNATURE_DESC COMMAND_SIGNATURES[] = 
        {
            {
                .ByteStride = sizeof(D3D12_DRAW_ARGUMENTS),
                .NumArgumentDescs = 1,
                .pArgumentDescs = &COMMAND_SIGNATURE_ARGUMENTS[0]
            },

            {
                .ByteStride = sizeof(D3D12_DRAW_INDEXED_ARGUMENTS),
                .NumArgumentDescs = 1,
                .pArgumentDescs = &COMMAND_SIGNATURE_ARGUMENTS[1]
            },

            {
                .ByteStride = sizeof(D3D12_DISPATCH_ARGUMENTS),
                .NumArgumentDescs = 1,
                .pArgumentDescs = &COMMAND_SIGNATURE_ARGUMENTS[2]
            },

            {
                .ByteStride = sizeof(D3D12_DISPATCH_MESH_ARGUMENTS),
                .NumArgumentDescs = 1,
                .pArgumentDescs = &COMMAND_SIGNATURE_ARGUMENTS[3]
            },

            {
                .ByteStride = sizeof(D3D12_DISPATCH_RAYS_DESC),
                .NumArgumentDescs = 1,
                .pArgumentDescs = &COMMAND_SIGNATURE_ARGUMENTS[4]
            },
        };
        RN_MATCH_ENUM_AND_ARRAY(COMMAND_SIGNATURES, CommandSignatureType);

        void BuildCommandSignatures(ID3D12Device10* device, ID3D12CommandSignature** outSignatures)
        {
            for (int i = 0; i < int(CommandSignatureType::Count); ++i)
            {
                RN_ASSERT(SUCCEEDED(device->CreateCommandSignature(&COMMAND_SIGNATURES[i], nullptr, IID_PPV_ARGS(&outSignatures[i]))));
            }
        }
    }

    DeviceD3D12::DeviceD3D12(const DeviceD3D12Options& options, const DeviceMemorySettings& memorySettings)
        : _rasterPipelines(MemoryCategory::D3D12, memorySettings.rasterPipelineCapacity)
        , _computePipelines(MemoryCategory::D3D12, memorySettings.computePipelineCapacity)
        , _rtPipelines(MemoryCategory::D3D12, memorySettings.raytracingPipelineCapacity)
        , _gpuAllocations(MemoryCategory::D3D12, memorySettings.gpuAllocationCapacity)
        , _buffers(MemoryCategory::D3D12, memorySettings.bufferCapacity)
        , _texture2Ds(MemoryCategory::D3D12, memorySettings.texture2dCapacity)
        , _texture3Ds(MemoryCategory::D3D12, memorySettings.texture3dCapacity)
        , _blases(MemoryCategory::D3D12, memorySettings.blasCapacity)
    {
        if (options.enableDebugLayer)
        {
            _d3dDebug = EnableD3D12DebugLayer(options.enableGPUValidation);
        }

        _dxgiFactory = CreateDXGIFactory();
        _dxgiAdapter = GetDXGIAdapterAtIndex(_dxgiFactory, options.adapterIndex);
        _d3dDevice = CreateD3D12Device(_dxgiAdapter);

        _caps = EnumerateDeviceCaps(_d3dDevice);
        
        _bindlessRootSignature = CreateBindlessRootSignature(_d3dDevice);

        if (_caps.hasHardwareRaytracing)
        {
            _rtLocalRootSignature = CreateRayTracingLocalRootSignature(_d3dDevice);
        }

        _resourceDescriptorHeap.InitAsResourceHeap(_d3dDevice);
        _samplerDescriptorHeap.InitAsSamplerHeap(_d3dDevice);
        _rtvDescriptorHeap.InitAsRTVHeap(_d3dDevice);
        _dsvDescriptorHeap.InitAsDSVHeap(_d3dDevice);

        _graphicsQueue = CreateGraphicsQueue(_d3dDevice);
        _graphicsFence = CreateFence(_d3dDevice);

        BuildCommandSignatures(_d3dDevice, _commandSignatures);
    }

    namespace
    {
        template <typename T>
        void SafeRelease(T*& ptr)
        {
            if (ptr)
            {
                ptr->Release();
                ptr = nullptr;
            }
        }
    }

    DeviceD3D12::~DeviceD3D12()
    {
        for (const auto& p : _uploadAllocators)
        {
            p.second->Reset();
        }

        for (const auto& p : _readbackAllocators)
        {
            p.second->Reset();
        }
        EndFrame();
        WaitForGraphicsQueueFence(_lastSubmittedGraphicsFenceValue);

        // Wrap up all outstanding work
        for (uint32_t i = 0; i < MAX_FRAME_LATENCY; ++i)
        {
            _gpuFrameFinalizerQueues[i].Flush(this);
        }

        _commandListPool.Reset();

        SafeRelease(_graphicsFence);
        SafeRelease(_graphicsQueue);
        SafeRelease(_rtLocalRootSignature);
        SafeRelease(_bindlessRootSignature);
        SafeRelease(_d3dDevice);
        SafeRelease(_dxgiAdapter);
        SafeRelease(_dxgiFactory);

        if (_d3dDebug)
        {
            IDXGIDebug* dxgiDebug = nullptr;
            RN_ASSERT(SUCCEEDED(DXGIGetDebugInterface1(0, __uuidof(IDXGIDebug), (void**)&dxgiDebug)));
            dxgiDebug->ReportLiveObjects(DXGI_DEBUG_DX, DXGI_DEBUG_RLO_DETAIL);
            dxgiDebug->Release();
        }
        SafeRelease(_d3dDebug);
    }

    uint64_t DeviceD3D12::SignalGraphicsQueueFence()
    {
        ++_lastSubmittedGraphicsFenceValue;
        _graphicsQueue->Signal(_graphicsFence, _lastSubmittedGraphicsFenceValue);

        return _lastSubmittedGraphicsFenceValue;
    }

    void DeviceD3D12::WaitForGraphicsQueueFence(uint64_t fence)
    {
        if (fence > _lastSeenGraphicsFenceValue)
        {
            _lastSeenGraphicsFenceValue = _graphicsFence->GetCompletedValue();
            while (fence > _lastSeenGraphicsFenceValue)
            {
                std::this_thread::yield();
                _lastSeenGraphicsFenceValue = _graphicsFence->GetCompletedValue();
            }
        }
    }

    void DeviceD3D12::EndFrame()
    {
        _signalledFrameGraphicsFenceValues[_frameIndex % MAX_FRAME_LATENCY] = SignalGraphicsQueueFence();

        uint64_t nextFrameIndex = _frameIndex + 1;
        uint64_t fence = _signalledFrameGraphicsFenceValues[nextFrameIndex % MAX_FRAME_LATENCY];
        WaitForGraphicsQueueFence(fence);

        // Execute on all queued up finalizer actions
        _gpuFrameFinalizerQueues[nextFrameIndex % MAX_FRAME_LATENCY].Flush(this);

        // Reset upload allocator pages
        for (const auto& p : _uploadAllocators)
        {
            p.second->Flush(nextFrameIndex);
        }

        for (const auto& p : _readbackAllocators)
        {
            p.second->Flush(nextFrameIndex);
        }

        // Reset command allocators
        _commandListPool.ResetAllocators(nextFrameIndex);

        _frameIndex = nextFrameIndex;
    }

    SwapChain* DeviceD3D12::CreateSwapChain(
        void* windowHandle, 
        RenderTargetFormat format,
        uint32_t width, 
        uint32_t height,
        uint32_t bufferCount,
        PresentMode presentMode)
    {
        return TrackedNew<SwapChainD3D12>(MemoryCategory::RHI, 
            this, 
            _graphicsQueue, 
            _dxgiFactory, 
            (HWND)windowHandle, 
            format, 
            width, 
            height, 
            bufferCount, 
            presentMode);
    }

    void DeviceD3D12::Destroy(SwapChain* swapChain)
    {
        TrackedDelete(static_cast<SwapChainD3D12*>(swapChain));
    }

    void DeviceD3D12::DrainGPU()
    {
        uint64_t fence = SignalGraphicsQueueFence();
        WaitForGraphicsQueueFence(fence);

        for (uint32_t i = 0; i < MAX_FRAME_LATENCY; ++i)
        {
            _gpuFrameFinalizerQueues[i].Flush(this);
        }
    }

    CommandList* DeviceD3D12::AllocateCommandList()
    {
        return _commandListPool.GetCommandList(
            this, 
            _frameIndex,
            GetTemporaryResourceAllocatorForCurrentThread(),
            GetReadbackAllocatorForCurrentThread(),
            _bindlessRootSignature,
            _resourceDescriptorHeap.D3DHeap(),
            _samplerDescriptorHeap.D3DHeap());
    }

    void DeviceD3D12::SubmitCommandLists(Span<CommandList*> cls)
    {
        Span<CommandListD3D12*> d3d12CLs(reinterpret_cast<CommandListD3D12**>(cls.data()), cls.size());
        for (CommandListD3D12* cl : d3d12CLs)
        {
            cl->Finalize();
        }

        MemoryScope SCOPE;
        ScopedVector<ID3D12CommandList*> nativeCLs;
        nativeCLs.reserve(cls.size());

        for (CommandListD3D12* cl : d3d12CLs)
        {
            nativeCLs.push_back(cl->D3DCommandList());
        }

        _graphicsQueue->ExecuteCommandLists(UINT(nativeCLs.size()), nativeCLs.data());
        SignalGraphicsQueueFence();

        for (CommandListD3D12* cl : d3d12CLs)
        {
            _commandListPool.ReturnCommandList(cl);
        }
    }

    void DeviceD3D12::QueueFrameFinalizerAction(FnOnFinalize fn, void* data)
    {
        FinalizerQueue& queue = _gpuFrameFinalizerQueues[_frameIndex % MAX_FRAME_LATENCY];
        queue.Queue(fn, data);
    }

    void* DeviceD3D12::MapBuffer(Buffer buffer, uint64_t offset, uint64_t size)
    {
        ID3D12Resource* resource = Resolve(buffer);

        void* ptr = nullptr;
        RN_ASSERT(SUCCEEDED(resource->Map(0, nullptr, &ptr)));
        return static_cast<void*>(static_cast<unsigned char*>(ptr) + offset);
    }

    void DeviceD3D12::UnmapBuffer(Buffer buffer, uint64_t offset, uint64_t size)
    {
        ID3D12Resource* resource = Resolve(buffer);
        resource->Unmap(0, nullptr);
    }

}