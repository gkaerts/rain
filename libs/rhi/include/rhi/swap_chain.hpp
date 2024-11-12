#pragma once

#include "common/common.hpp"
#include "common/handle.hpp"

#include "rhi/handles.hpp"

namespace rn::rhi
{
    struct BackBuffer
    {
        Texture2D texture;
        RenderTargetView view;
    };

    class SwapChain
    {
    public:

        virtual ~SwapChain() {}

        virtual void Resize(uint32_t width, uint32_t height) { RN_NOT_IMPLEMENTED(); }
        virtual void WaitForRender() { RN_NOT_IMPLEMENTED(); }

        virtual void Present() { RN_NOT_IMPLEMENTED(); }
        virtual BackBuffer CurrentBackBuffer() { RN_NOT_IMPLEMENTED(); return {}; }
    };
}