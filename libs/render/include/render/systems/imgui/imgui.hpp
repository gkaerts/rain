#pragma once

#include "common/common.hpp"
#include "rhi/handles.hpp"
#include "rg/resource.hpp"

#if RN_PLATFORM_WINDOWS
    struct SDL_Window;
#endif

struct ImFont;
struct ImGuiViewport;
struct ImDrawData;

namespace rn::rhi
{
    class Device;
    class CommandList;
    enum class RenderTargetFormat : uint32_t;
}

namespace rn::rg
{
    class RenderGraph;
}

namespace rn::render
{
    enum class ImGuiFontType : uint32_t
    {
        Default,
        IconsSmall,
        IconsLarge,

        Count
    };

    struct ImGuiRendererDesc
    {
        rhi::RenderTargetFormat outputFormat;
        SDL_Window*             window;
    };

    struct ImGuiRenderInputs
    {
        rg::Texture2D   output;
        ImDrawData*     drawData;
    };

    class ImGuiRenderer
    {
    public:

        ImGuiRenderer(rhi::Device* device, rhi::CommandList* uploadCL, const ImGuiRendererDesc& desc);
        ~ImGuiRenderer();

        void BeginFrame(float dpiScale);
        void Render(rg::RenderGraph* graph, const ImGuiRenderInputs& inputs);
        void RenderViewportWindows();

    private:

        rhi::Device*        _device = nullptr;
        rg::RenderGraph*    _viewportRenderGraph = nullptr;
        rhi::GPUAllocation  _fontAllocation = rhi::GPUAllocation::Invalid;
        rhi::Texture2D      _fontTexture = rhi::Texture2D::Invalid;
        rhi::Texture2DView  _fontTextureView = rhi::Texture2DView::Invalid;
        rhi::RasterPipeline _pipeline = rhi::RasterPipeline::Invalid;
        float               _lastDPIScale = 1.0f;

        ImFont*             _fonts[CountOf<ImGuiFontType>()] = {};
    };
}

