#pragma once

#define NOMINMAX
#include <dxgi1_6.h>
#include "rhi/swap_chain.hpp"
#include "rhi/handles.hpp"
#include "rhi/limits.hpp"
#include "rhi/format.hpp"
#include "d3d12_fwd.hpp"


namespace rn::rhi
{
    enum class PresentMode : uint32_t;
    class DeviceD3D12;
    class SwapChainD3D12 : public SwapChain
    {
    public:

        SwapChainD3D12(
            DeviceD3D12* device,
            ID3D12CommandQueue* graphicsQueue,
            IDXGIFactory7* dxgiFactory, 
            HWND windowHandle, 
            RenderTargetFormat format,
            uint32_t width, 
            uint32_t height,
            uint32_t bufferCount,
            PresentMode presentMode);

        ~SwapChainD3D12();

        void Resize(uint32_t width, uint32_t height) override;
        void WaitForRender() override;

        void Present() override;
        BackBuffer CurrentBackBuffer() override;

    private:
        void RegisterResources();
        void DestroyResources();

        DeviceD3D12* _device = nullptr;
        IDXGISwapChain3* _swapChain = nullptr;
        HANDLE _waitableObject = nullptr;
        BackBuffer _backBuffers[MAX_BACKBUFFER_COUNT] = {};

        DXGI_SWAP_CHAIN_DESC1 _desc = {};
        PresentMode _presentMode;
    };
}