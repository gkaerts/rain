#include "device_d3d12.hpp"
#include "pipeline_d3d12.hpp"
#include "rhi/rhi_d3d12.hpp"

#include "common/memory/memory.hpp"
#include "common/log/log.hpp"

#include "d3d12.h"
#include <dxgi1_6.h>

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
        SafeRelease(_rtLocalRootSignature);
        SafeRelease(_bindlessRootSignature);
        SafeRelease(_d3dDevice);
        SafeRelease(_dxgiAdapter);
        SafeRelease(_dxgiFactory);
        SafeRelease(_d3dDebug);
    }

    void DeviceD3D12::QueueFrameFinalizerAction(FnOnFinalize fn, void* data)
    {
        FinalizerQueue& queue = _gpuFrameFinalizerQueues[_frameIndex % MAX_FRAME_LATENCY];
        queue.Queue(fn, data);
    }

}