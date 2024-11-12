#include "swap_chain_d3d12.hpp"
#include "format_d3d12.hpp"
#include "device_d3d12.hpp"
#include "d3d12.h"

#include "rhi/format.hpp"

namespace rn::rhi
{

    SwapChainD3D12::SwapChainD3D12(
        DeviceD3D12* device,
        ID3D12CommandQueue* graphicsQueue,
        IDXGIFactory7* dxgiFactory, 
        HWND windowHandle, 
        RenderTargetFormat format,
        uint32_t width, 
        uint32_t height,
        uint32_t bufferCount,
        PresentMode presentMode)
        : _device(device)
        , _presentMode(presentMode)
    {
        RN_ASSERT(bufferCount <= MAX_BACKBUFFER_COUNT);
        _desc = {
            .Width = width,
            .Height = height,
            .Format = ToDXGIFormat(format),
            .SampleDesc = { 1, 0 },
            .BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT,
            .BufferCount = bufferCount,
            .Scaling = DXGI_SCALING_STRETCH,
            .SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD,
            .Flags = DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT,
        };

        {
            IDXGISwapChain1* dxgi1SwapChain = nullptr;
            RN_ASSERT(SUCCEEDED(dxgiFactory->CreateSwapChainForHwnd(
                graphicsQueue,
                windowHandle,
                &_desc,
                nullptr,
                nullptr,
                &dxgi1SwapChain)));

            RN_ASSERT(SUCCEEDED(dxgi1SwapChain->QueryInterface(&_swapChain)));
            dxgi1SwapChain->Release();
        }
        
        _waitableObject = _swapChain->GetFrameLatencyWaitableObject();

        RegisterResources();
    }

    void SwapChainD3D12::RegisterResources()
    {
        for (uint32_t i = 0; i < _desc.BufferCount; ++i)
        {
            ID3D12Resource* texture = nullptr;
            RN_ASSERT(SUCCEEDED(_swapChain->GetBuffer(i, __uuidof(ID3D12Resource), (void**)(&texture))));

            Texture2D tex = _device->PlaceTexture2D(texture);
            _backBuffers[i] = {
                .texture = tex,
                .view = _device->CreateRenderTargetView({
                    .texture = tex,
                    .format = ToRenderTargetEquivalent(DXGIToTextureFormat(_desc.Format))   // I'm lazy :(
                }),
            };
        }
    }

    SwapChainD3D12::~SwapChainD3D12()
    {
        DestroyResources();
    }

    void SwapChainD3D12::DestroyResources()
    {
        for (uint32_t i = 0; i < _desc.BufferCount; ++i)
        {
            _device->Destroy(_backBuffers[i].view);
            _device->Destroy(_backBuffers[i].texture);

            _backBuffers[i] = {};
        }
    }


    void SwapChainD3D12::Resize(uint32_t width, uint32_t height)
    {
        if (width != _desc.Width || height != _desc.Height)
        {
            DestroyResources();
            _device->DrainGPU();

            _desc.Width = width;
            _desc.Height = height;

            RN_ASSERT(SUCCEEDED(_swapChain->ResizeBuffers(_desc.BufferCount, _desc.Width, _desc.Height, _desc.Format, _desc.Flags)));
            RegisterResources();
        }
    }

    void SwapChainD3D12::WaitForRender()
    {
        RN_ASSERT(WaitForSingleObjectEx(
            _waitableObject,
            1000,
            true) == WAIT_OBJECT_0);
    }

    void SwapChainD3D12::Present()
    {
        RN_ASSERT(SUCCEEDED(_swapChain->Present(
            _presentMode == PresentMode::VSync ? 1 : 0,
            0)));
    }

    BackBuffer SwapChainD3D12::CurrentBackBuffer()
    {
        return _backBuffers[_swapChain->GetCurrentBackBufferIndex()];
    }

}