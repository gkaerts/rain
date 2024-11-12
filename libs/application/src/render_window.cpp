#include "app/render_window.hpp"

#include "rhi/swap_chain.hpp"
#include "rhi/device.hpp"
#include "rhi/format.hpp"

#include "common/memory/memory.hpp"

namespace rn::app
{
    RenderWindow::RenderWindow(rhi::Device* rhiDevice, const char* title, uint32_t width, uint32_t height, rhi::PresentMode presentMode)
    {
        InitializeWindow(title, width, height);
        _rhiSwapChain = rhiDevice->CreateSwapChain(
            NativeWindow(), 
            rhi::RenderTargetFormat::RGBA8Unorm,
            width,
            height,
            2,
            presentMode);
    }

    RenderWindow::~RenderWindow()
    {
        TrackedDelete(_rhiSwapChain);
        TeardownWindow();
    }

    void RenderWindow::OnResize()
    {
        uint2 drawableSize = DrawableSize();
        _rhiSwapChain->Resize(drawableSize.x, drawableSize.y);
    }
}