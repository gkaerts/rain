#include "render/systems/imgui/imgui.hpp"
#include "render/systems/imgui/icons.hpp"

#include "backends/imgui_impl_sdl2.h"
#include "SDL.h"

#include "common/memory/memory.hpp"

#include "rhi/device.hpp"
#include "rhi/command_list.hpp"
#include "rhi/format.hpp"
#include "rhi/swap_chain.hpp"

#include "rg/render_graph.hpp"
#include "rg/render_pass_context.hpp"

#include "shared/imgui/imgui.h"
#include "imgui.h"

#include "font_opensans_regular.hpp"
#include "font_quicksand_regular.hpp"
#include "font_material_icons_regular.hpp"

using namespace std::literals;

namespace rn::render
{
    RN_DEFINE_MEMORY_CATEGORY(ImGui);

    namespace ImGuiShader
    {
        #include "imgui/imgui.d3d12.hpp"
    }

    namespace
    {
        ImGuiRenderer* ImGuiRendererFromContext()
        {
            return ImGui::GetCurrentContext() ? 
                static_cast<ImGuiRenderer*>(ImGui::GetIO().BackendRendererUserData) : 
                nullptr;
        }

        ImGuiStyle GetStyle()
        {   
            ImGuiStyle style;
            ImVec4* colors = style.Colors;
			colors[ImGuiCol_Text]                   = ImVec4(0.90f, 0.90f, 0.90f, 1.00f);
			colors[ImGuiCol_TextDisabled]           = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
			colors[ImGuiCol_WindowBg]               = ImVec4(0.10f, 0.10f, 0.10f, 1.00f);
			colors[ImGuiCol_ChildBg]                = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
			colors[ImGuiCol_PopupBg]                = ImVec4(0.19f, 0.19f, 0.19f, 0.92f);
			colors[ImGuiCol_Border]                 = ImVec4(0.19f, 0.19f, 0.19f, 0.29f);
			colors[ImGuiCol_BorderShadow]           = ImVec4(0.00f, 0.00f, 0.00f, 0.24f);
			colors[ImGuiCol_FrameBg]                = ImVec4(0.05f, 0.05f, 0.05f, 0.54f);
			colors[ImGuiCol_FrameBgHovered]         = ImVec4(0.19f, 0.19f, 0.19f, 0.54f);
			colors[ImGuiCol_FrameBgActive]          = ImVec4(0.20f, 0.22f, 0.23f, 1.00f);
			colors[ImGuiCol_TitleBg]                = ImVec4(0.00f, 0.00f, 0.00f, 1.00f);
			colors[ImGuiCol_TitleBgActive]          = ImVec4(0.06f, 0.06f, 0.06f, 1.00f);
			colors[ImGuiCol_TitleBgCollapsed]       = ImVec4(0.00f, 0.00f, 0.00f, 1.00f);
			colors[ImGuiCol_MenuBarBg]              = ImVec4(0.14f, 0.14f, 0.14f, 1.00f);
			colors[ImGuiCol_ScrollbarBg]            = ImVec4(0.05f, 0.05f, 0.05f, 0.54f);
			colors[ImGuiCol_ScrollbarGrab]          = ImVec4(0.34f, 0.34f, 0.34f, 0.54f);
			colors[ImGuiCol_ScrollbarGrabHovered]   = ImVec4(0.40f, 0.40f, 0.40f, 0.54f);
			colors[ImGuiCol_ScrollbarGrabActive]    = ImVec4(0.56f, 0.56f, 0.56f, 0.54f);
			colors[ImGuiCol_CheckMark]              = ImVec4(0.33f, 0.67f, 0.86f, 1.00f);
			colors[ImGuiCol_SliderGrab]             = ImVec4(0.34f, 0.34f, 0.34f, 0.54f);
			colors[ImGuiCol_SliderGrabActive]       = ImVec4(0.56f, 0.56f, 0.56f, 0.54f);
			colors[ImGuiCol_Button]                 = ImVec4(0.05f, 0.05f, 0.05f, 0.54f);
			colors[ImGuiCol_ButtonHovered]          = ImVec4(0.19f, 0.19f, 0.19f, 0.54f);
			colors[ImGuiCol_ButtonActive]           = ImVec4(0.20f, 0.22f, 0.23f, 1.00f);
			colors[ImGuiCol_Header]                 = ImVec4(0.00f, 0.00f, 0.00f, 0.52f);
			colors[ImGuiCol_HeaderHovered]          = ImVec4(0.00f, 0.00f, 0.00f, 0.36f);
			colors[ImGuiCol_HeaderActive]           = ImVec4(0.20f, 0.22f, 0.23f, 0.33f);
			colors[ImGuiCol_Separator]              = ImVec4(0.28f, 0.28f, 0.28f, 0.29f);
			colors[ImGuiCol_SeparatorHovered]       = ImVec4(0.44f, 0.44f, 0.44f, 0.29f);
			colors[ImGuiCol_SeparatorActive]        = ImVec4(0.40f, 0.44f, 0.47f, 1.00f);
			colors[ImGuiCol_ResizeGrip]             = ImVec4(0.28f, 0.28f, 0.28f, 0.29f);
			colors[ImGuiCol_ResizeGripHovered]      = ImVec4(0.44f, 0.44f, 0.44f, 0.29f);
			colors[ImGuiCol_ResizeGripActive]       = ImVec4(0.40f, 0.44f, 0.47f, 1.00f);
			colors[ImGuiCol_Tab]                    = ImVec4(0.00f, 0.00f, 0.00f, 0.52f);
			colors[ImGuiCol_TabHovered]             = ImVec4(0.14f, 0.14f, 0.14f, 1.00f);
			colors[ImGuiCol_TabActive]              = ImVec4(0.20f, 0.20f, 0.20f, 0.36f);
			colors[ImGuiCol_TabUnfocused]           = ImVec4(0.00f, 0.00f, 0.00f, 0.52f);
			colors[ImGuiCol_TabUnfocusedActive]     = ImVec4(0.14f, 0.14f, 0.14f, 1.00f);
			colors[ImGuiCol_DockingPreview]         = ImVec4(0.33f, 0.67f, 0.86f, 1.00f);
			colors[ImGuiCol_DockingEmptyBg]         = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
			colors[ImGuiCol_PlotLines]              = ImVec4(1.00f, 0.00f, 0.00f, 1.00f);
			colors[ImGuiCol_PlotLinesHovered]       = ImVec4(1.00f, 0.00f, 0.00f, 1.00f);
			colors[ImGuiCol_PlotHistogram]          = ImVec4(1.00f, 0.00f, 0.00f, 1.00f);
			colors[ImGuiCol_PlotHistogramHovered]   = ImVec4(1.00f, 0.00f, 0.00f, 1.00f);
			colors[ImGuiCol_TableHeaderBg]          = ImVec4(0.00f, 0.00f, 0.00f, 0.52f);
			colors[ImGuiCol_TableBorderStrong]      = ImVec4(0.00f, 0.00f, 0.00f, 0.52f);
			colors[ImGuiCol_TableBorderLight]       = ImVec4(0.28f, 0.28f, 0.28f, 0.29f);
			colors[ImGuiCol_TableRowBg]             = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
			colors[ImGuiCol_TableRowBgAlt]          = ImVec4(1.00f, 1.00f, 1.00f, 0.06f);
			colors[ImGuiCol_TextSelectedBg]         = ImVec4(0.20f, 0.22f, 0.23f, 1.00f);
			colors[ImGuiCol_DragDropTarget]         = ImVec4(0.33f, 0.67f, 0.86f, 1.00f);
			colors[ImGuiCol_NavHighlight]           = ImVec4(1.00f, 0.00f, 0.00f, 1.00f);
			colors[ImGuiCol_NavWindowingHighlight]  = ImVec4(1.00f, 0.00f, 0.00f, 0.70f);
			colors[ImGuiCol_NavWindowingDimBg]      = ImVec4(1.00f, 0.00f, 0.00f, 0.20f);
			colors[ImGuiCol_ModalWindowDimBg]       = ImVec4(1.00f, 0.00f, 0.00f, 0.35f);

			style.WindowPadding                     = ImVec2(8.00f, 8.00f);
			style.FramePadding                      = ImVec2(5.00f, 5.00f);
			style.CellPadding                       = ImVec2(6.00f, 6.00f);
			style.ItemSpacing                       = ImVec2(6.00f, 6.00f);
			style.ItemInnerSpacing                  = ImVec2(6.00f, 6.00f);
			style.TouchExtraPadding                 = ImVec2(0.00f, 0.00f);
			style.IndentSpacing                     = 25;
			style.ScrollbarSize                     = 15;
			style.GrabMinSize                       = 10;
			style.WindowBorderSize                  = 1;
			style.ChildBorderSize                   = 1;
			style.PopupBorderSize                   = 1;
			style.FrameBorderSize                   = 1;
			style.TabBorderSize                     = 0;
			style.WindowRounding                    = 7;
			style.ChildRounding                     = 4;
			style.FrameRounding                     = 3;
			style.PopupRounding                     = 4;
			style.ScrollbarRounding                 = 9;
			style.GrabRounding                      = 3;
			style.LogSliderDeadzone                 = 4;
			style.TabRounding                       = 4;

            return style;
        }

        void ApplyStyle()
        {
            ImGui::GetStyle() = GetStyle();
        }

        void CreateFonts(rhi::Device* device, 
            rhi::CommandList* cl,
            float scale,
            rhi::GPUAllocation& outAllocation,
            rhi::Texture2D& outTexture,
            rhi::Texture2DView& outView,
            ImFont** outFonts)
        {
            ImGuiIO& io = ImGui::GetIO();

            if (IsValid(outView))
            {
                device->Destroy(outView);
            }

            if (IsValid(outTexture))
            {
                device->Destroy(outTexture);
            }

            if (IsValid(outAllocation))
            {
                device->GPUFree(outAllocation);
            }

            // Imgui needs ownership over the TTF pointer, so copy it over. It will free it up when it's done with it
            void* defaultFontPtr =  IM_ALLOC(sizeof(fonts::FONT_QUICKSAND_REGULAR));
            void* iconLargePtr =    IM_ALLOC(sizeof(fonts::FONT_MATERIAL_ICONS_REGULAR));
            void* iconSmallPtr =    IM_ALLOC(sizeof(fonts::FONT_MATERIAL_ICONS_REGULAR));

            std::memcpy(defaultFontPtr, fonts::FONT_QUICKSAND_REGULAR, sizeof(fonts::FONT_QUICKSAND_REGULAR));
            std::memcpy(iconLargePtr, fonts::FONT_MATERIAL_ICONS_REGULAR, sizeof(fonts::FONT_MATERIAL_ICONS_REGULAR));
            std::memcpy(iconSmallPtr, fonts::FONT_MATERIAL_ICONS_REGULAR, sizeof(fonts::FONT_MATERIAL_ICONS_REGULAR));

            ImFontConfig fontConfig;
            fontConfig.OversampleH = 3;
            fontConfig.OversampleV = 3;
            
            io.Fonts->Clear();
            outFonts[int(ImGuiFontType::Default)] = io.Fonts->AddFontFromMemoryTTF(
                defaultFontPtr,
                sizeof(fonts::FONT_QUICKSAND_REGULAR),
                roundf(16.f * scale),
                &fontConfig);

            const ImWchar iconRanges[] = { ImWchar(ICON_MIN_MD), ImWchar(ICON_MAX_MD), 0 };
            outFonts[int(ImGuiFontType::IconsLarge)] = io.Fonts->AddFontFromMemoryTTF(
                iconLargePtr,
                sizeof(fonts::FONT_MATERIAL_ICONS_REGULAR),
                roundf(25.f * scale),
                &fontConfig,
                iconRanges);

            outFonts[int(ImGuiFontType::IconsSmall)] = io.Fonts->AddFontFromMemoryTTF(
                iconSmallPtr,
                sizeof(fonts::FONT_MATERIAL_ICONS_REGULAR),
                roundf(16.f * scale),
                &fontConfig,
                iconRanges);

            io.Fonts->Build();
            
            
			unsigned char* pixels = nullptr;
			int width = 0, height = 0;
			io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);

            const rhi::Texture2DDesc fontTextureDesc = {
                .flags = rhi::TextureCreationFlags::AllowShaderReadOnly,
                .width = uint32_t(width),
                .height = uint32_t(height),
                .arraySize = 1,
                .mipLevels = 1,
                .format = rhi::TextureFormat::RGBA8Unorm,
                .optClearValue = nullptr,
                .name = "ImGui Font Atlas"sv
            };

            rhi::ResourceFootprint footprint = device->CalculateTextureFootprint(fontTextureDesc);
            outAllocation = device->GPUAlloc(MemoryCategory::ImGui, footprint.sizeInBytes, rhi::GPUAllocationFlags::DeviceAccessOptimal);
            outTexture = device->CreateTexture2D(fontTextureDesc,  {
                .allocation = outAllocation, 
                .offsetInAllocation = 0,
                .regionSize = footprint.sizeInBytes 
            });

            const rhi::Texture2DViewDesc fontTextureViewDesc = {
                .texture = outTexture,
                .format = fontTextureDesc.format,
                .mipCount = 1,
                .minLODClamp = 0.0f
            };

            outView = device->CreateTexture2DView(fontTextureViewDesc);
            io.Fonts->SetTexID(ImTextureID(outView));

            rhi::MipUploadDesc mipUploads[1] = {};
            device->CalculateMipUploadDescs(fontTextureDesc, mipUploads);

            cl->Barrier({
                .texture2DBarriers = {
                    {
                        .fromStage = rhi::PipelineSyncStage::None,
                        .toStage = rhi::PipelineSyncStage::Copy,
                        .fromAccess = rhi::PipelineAccess::None,
                        .toAccess = rhi::PipelineAccess::CopyWrite,
                        .fromLayout = rhi::TextureLayout::Undefined,
                        .toLayout = rhi::TextureLayout::CopyWrite,
                        .handle = outTexture,
                        .firstMipLevel = 0,
                        .numMips = 1,
                        .firstArraySlice = 0,
                        .numArraySlices = 1
                    }
                }
            });

            Span<uint8_t> pixelData = { static_cast<uint8_t*>(pixels), width * height * sizeof(uint32_t) };
            cl->UploadTextureData(outTexture, 0, mipUploads, pixelData);

            cl->Barrier({
                .texture2DBarriers = {
                    {
                        .fromStage = rhi::PipelineSyncStage::Copy,
                        .toStage = rhi::PipelineSyncStage::PixelShader,
                        .fromAccess = rhi::PipelineAccess::CopyWrite,
                        .toAccess = rhi::PipelineAccess::ShaderRead,
                        .fromLayout = rhi::TextureLayout::CopyWrite,
                        .toLayout = rhi::TextureLayout::ShaderRead,
                        .handle = outTexture,
                        .firstMipLevel = 0,
                        .numMips = 1,
                        .firstArraySlice = 0,
                        .numArraySlices = 1
                    }
                }
            });
        }

        void InitImGuiBackend(ImGuiRenderer* imguiRenderer, SDL_Window* window)
        {
            ImGuiIO& io = ImGui::GetIO();
            io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
            io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;

            ImGui_ImplSDL2_InitForOther(window);

            io.BackendRendererName = "imgui_impl_rain";
            io.BackendRendererUserData = imguiRenderer;
            io.BackendFlags |= ImGuiBackendFlags_RendererHasVtxOffset;  // We can honor the ImDrawCmd::VtxOffset field, allowing for large meshes.
            io.BackendFlags |= ImGuiBackendFlags_RendererHasViewports;

            io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
            io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
        }
    }

    ImGuiRenderer::ImGuiRenderer(rhi::Device* device, rhi::CommandList* uploadCL, const ImGuiRendererDesc& desc)
        : _device(device)
    {
        _viewportRenderGraph = TrackedNew<rg::RenderGraph>(MemoryCategory::ImGui, device);

        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ApplyStyle();

        InitImGuiBackend(this, desc.window);
        CreateFonts(device, uploadCL, _lastDPIScale, _fontAllocation, _fontTexture, _fontTextureView, _fonts);

        _pipeline = device->CreateRasterPipeline({
            .flags = rhi::RasterPipelineFlags::None,
            .vertexShaderBytecode = ImGuiShader::VS,
            .pixelShaderBytecode = ImGuiShader::PS,
            .rasterizerState = rhi::RasterizerState::SolidNoCull,
            .blendState = rhi::BlendState::AlphaBlend,
            .depthState = rhi::DepthState::Disabled,
            .renderTargetFormats = {
                desc.outputFormat
            },
            .depthFormat = rhi::DepthFormat::None,
            .topology = rhi::TopologyType::TriangleList
        });

        ImGuiPlatformIO& platform_io = ImGui::GetPlatformIO();
        platform_io.Renderer_CreateWindow = [](ImGuiViewport* viewport)
        {
            ImGuiRenderer* imguiRenderer = ImGuiRendererFromContext();

    #if RN_PLATFORM_WINDOWS
            // PlatformHandleRaw should always be a HWND, whereas PlatformHandle might be a higher-level handle (e.g. GLFWWindow*, SDL_Window*).
            // Some backend will leave PlatformHandleRaw NULL, in which case we assume PlatformHandle will contain the HWND.
            void* nativeWindow = viewport->PlatformHandleRaw ? viewport->PlatformHandleRaw : viewport->PlatformHandle;
            IM_ASSERT(nativeWindow != 0);
    #endif
            rhi::Device* device = imguiRenderer->_device;
            rhi::SwapChain* swapChain = device->CreateSwapChain(nativeWindow,
                rhi::RenderTargetFormat::RGBA8Unorm,
                uint32_t(viewport->Size.x),
                uint32_t(viewport->Size.y),
                2,
                rhi::PresentMode::Immediate);

            viewport->RendererUserData = swapChain;
        };


        platform_io.Renderer_DestroyWindow = [](ImGuiViewport* viewport)
        {
            // The main viewport (owned by the application) will always have RendererUserData == NULL since we didn't create the data for it.
            if (rhi::SwapChain* swapChain = static_cast<rhi::SwapChain*>(viewport->RendererUserData))
            {
                ImGuiRenderer* renderer = ImGuiRendererFromContext();
                renderer->_device->Destroy(swapChain);
            }
            viewport->RendererUserData = nullptr;
        };

        platform_io.Renderer_SetWindowSize = [](ImGuiViewport* viewport, ImVec2 size)
		{
			if (rhi::SwapChain* swapChain = static_cast<rhi::SwapChain*>(viewport->RendererUserData))
			{
                swapChain->Resize(uint32_t(size.x), uint32_t(size.y));
			}
		};

        platform_io.Renderer_RenderWindow = [](ImGuiViewport* viewport, void*)
		{
            ImGuiRenderer* renderer = ImGuiRendererFromContext();
            rg::RenderGraph* graph = renderer->_viewportRenderGraph;

            rhi::SwapChain* swapChain = static_cast<rhi::SwapChain*>(viewport->RendererUserData);
            rhi::BackBuffer bb = swapChain->CurrentBackBuffer();

            graph->Reset({
                .x = 0,
                .y = 0,
                .width = uint32_t(viewport->Size.x),
                .height = uint32_t(viewport->Size.y)
            });

            rg::Texture2D output = graph->RegisterTexture2D({
                .texture = bb.texture,
                .view = rhi::Texture2DView::Invalid,
                .rwViews = {},
                .rtv = bb.view,
                .dsv = rhi::DepthStencilView::Invalid,
                .clearValue = rhi::ClearColor(0.0f, 0.0f, 0.0f, 1.0f),
                .lastSyncStage = rhi::PipelineSyncStage::None,
                .lastAccess = rhi::PipelineAccess::None,
                .lastLayout = rhi::TextureLayout::Present
            });

            renderer->Render(graph, {
                .output = output,
                .drawData = viewport->DrawData
            });

            graph->AddRenderPass<void>({
                .name = "Prepare ImGui viewport for presentation",
                .flags = rg::RenderPassFlags::IsSmall | rg::RenderPassFlags::ComputeOnly,
                .texture2Ds = {
                    rg::Present(output)
                }
            }, nullptr);

            graph->Build();
            graph->Execute(rg::RenderGraphExecutionFlags::ForceSingleThreaded);
		};

        platform_io.Renderer_SwapBuffers = [](ImGuiViewport* viewport, void*)
		{
            rhi::SwapChain* swapChain = static_cast<rhi::SwapChain*>(viewport->RendererUserData);
            swapChain->Present();
		};
    }

    ImGuiRenderer::~ImGuiRenderer()
    {
        ImGui::DestroyPlatformWindows();
        if (IsValid(_pipeline))
        {
            _device->Destroy(_pipeline);
        }

        if (IsValid(_fontTextureView))
        {
            _device->Destroy(_fontTextureView);
        }
        
        if (IsValid(_fontTexture))
        {
            _device->Destroy(_fontTexture);
        }

        if (IsValid(_fontAllocation))
        {
            _device->GPUFree(_fontAllocation);
        }

        TrackedDelete(_viewportRenderGraph);
    }

    void ImGuiRenderer::BeginFrame(float dpiScale)
    {
        ImGui_ImplSDL2_NewFrame();
        
        if (dpiScale > _lastDPIScale + FLT_EPSILON || 
            dpiScale < _lastDPIScale - FLT_EPSILON)
        {
            rhi::CommandList* uploadCL = _device->AllocateCommandList();
            CreateFonts(_device, uploadCL, dpiScale, _fontAllocation, _fontTexture, _fontTextureView, _fonts);
            _device->SubmitCommandLists({&uploadCL, 1});
        }
        
        _lastDPIScale = dpiScale;

        ImGui::NewFrame();
        ApplyStyle();
        ImGui::GetStyle().ScaleAllSizes(dpiScale);
    }

    void ImGuiRenderer::Render(rg::RenderGraph* graph, const ImGuiRenderInputs& inputs)
    {
        ImDrawData* drawData = inputs.drawData;
        if (!drawData)
        {
            ImGui::Render();
            drawData = ImGui::GetDrawData();
        }

        if (drawData->DisplaySize.x <= 0.0f || drawData->DisplaySize.y <= 0.0f)
        {
            return;
        }
        
        int drawCount = 0;
		for (int n = 0; n < drawData->CmdListsCount; n++)
		{
			const ImDrawList* drawList = drawData->CmdLists[n];
			drawCount += drawList->CmdBuffer.Size;
		}

        struct ImGuiGraphData
        {
            rg::Buffer vertexBuffer;
            rg::Buffer indexBuffer;
            rg::Buffer drawBuffer;
            ImDrawData* drawData;
            rhi::RasterPipeline pipeline;
        };

        ImGuiGraphData* graphData = graph->AllocateScratchPOD<ImGuiGraphData>();
        *graphData = {
            .vertexBuffer = graph->AllocateBuffer({
                .sizeInBytes = uint32_t(std::max(1, drawData->TotalVtxCount) * sizeof(ImGuiVertex)),
                .name = "ImGui Vertex Buffer"
            }),
            .indexBuffer = graph->AllocateBuffer({
                .sizeInBytes = uint32_t(std::max(1, drawData->TotalIdxCount) * sizeof(ImDrawIdx)),
                .name = "ImGui Index Buffer"
            }),
            .drawBuffer = graph->AllocateBuffer({
                .sizeInBytes = uint32_t(std::max(1, drawCount) * sizeof(ImGuiDrawData)),
                .name = "ImGui Draw Data Buffer"
            }),
            .drawData = drawData,
            .pipeline = _pipeline
        };

        graph->AddRenderPass<ImGuiGraphData>({
            .name = "Upload ImGui Data",
            .flags = rg::RenderPassFlags::IsSmall | rg::RenderPassFlags::ComputeOnly,
            .buffers = {
                rg::CopyTo(graphData->vertexBuffer),
                rg::CopyTo(graphData->indexBuffer),
                rg::CopyTo(graphData->drawBuffer)
            },
            .onExecute = [](rhi::Device*, rg::RenderPassContext* ctxt, rhi::CommandList* cl, const ImGuiGraphData* data, uint32_t)
            {
                ImDrawData* drawData = data->drawData;
                rg::BufferRegion vertexBuffer = ctxt->Resolve(data->vertexBuffer);
                rg::BufferRegion indexBuffer = ctxt->Resolve(data->indexBuffer);
                rg::BufferRegion drawBuffer = ctxt->Resolve(data->drawBuffer);

                uint32_t offsetInVB = vertexBuffer.offset;
                uint32_t offsetInIB = indexBuffer.offset;
                uint32_t offsetInDB = drawBuffer.offset;
                uint32_t drawBaseVertex = 0;

                MemoryScope SCOPE;
                ScopedVector<ImGuiDrawData> draws;
                for (int n = 0; n < drawData->CmdListsCount; ++n)
                {
                    const ImDrawList* drawList = drawData->CmdLists[n];
                    cl->UploadTypedBufferData<ImDrawVert>(vertexBuffer.buffer, offsetInVB, drawList->VtxBuffer);
                    cl->UploadTypedBufferData<ImDrawIdx>(indexBuffer.buffer, offsetInIB, drawList->IdxBuffer);

                    offsetInVB += drawList->VtxBuffer.Size * sizeof(ImDrawVert);
                    offsetInIB += drawList->IdxBuffer.Size * sizeof(ImDrawIdx);
                    for (int drawIdx = 0; drawIdx < drawList->CmdBuffer.Size; ++drawIdx)
				    {
                        const ImDrawCmd* pcmd = &drawList->CmdBuffer[drawIdx];
                        if (!pcmd->UserCallback)
                        {
                            rhi::Texture2DView textureView = rhi::Texture2DView(pcmd->TextureId);
                            draws.push_back({
                                .tex2d_Texture0 = rhi::ToShaderResource(textureView),
                                .baseVertex = uint32_t(drawBaseVertex + pcmd->VtxOffset)
                            });
                        }
                    }
                    drawBaseVertex += drawList->VtxBuffer.Size;
                }

                cl->UploadTypedBufferData<ImGuiDrawData>(drawBuffer.buffer, offsetInDB, draws);
            }
        }, graphData);

        graph->AddRenderPass<ImGuiGraphData>({
            .name = "Render ImGui",
            .flags = rg::RenderPassFlags::IsSmall,
            .colorAttachments = {
                { .texture = inputs.output, .loadOp = rhi::LoadOp::Load }
            },
            .buffers = {
                rg::ShaderReadOnly(graphData->vertexBuffer),
                rg::ShaderReadOnly(graphData->drawBuffer),
                rg::IndexBuffer(graphData->indexBuffer)
            },
            .onExecute = [](rhi::Device*, rg::RenderPassContext* ctxt, rhi::CommandList* cl, const ImGuiGraphData* data, uint32_t)
            {
                ImDrawData* drawData = data->drawData;
                float L = drawData->DisplayPos.x;
                float R = drawData->DisplayPos.x + drawData->DisplaySize.x;
                float T = drawData->DisplayPos.y;
                float B = drawData->DisplayPos.y + drawData->DisplaySize.y;
                ImGuiPassData passData = {
                    .projectionMatrix = {{
                        2.0f / (R - L),     0.0f,               0.0f,   0.0f,
                        0.0f,               2.0f / (T - B),     0.0f,   0.0f,
                        0.0f,               0.0f,               0.5f,   0.0f,
                        (R + L) / (L - R),  (T + B) / (B - T),  0.5f,   1.0f,
                    }},
                    .buffer_Vertices = rhi::ToShaderResource(ctxt->ResolveView(data->vertexBuffer)),
                    .buffer_DrawData = rhi::ToShaderResource(ctxt->ResolveView(data->drawBuffer))
                };

                rhi::UniformBufferView passDataView = ctxt->AllocateTemporaryUniformBufferView<ImGuiPassData>(passData);
                ImGuiConstants pushConstants = {
                    .uniform_PassData = rhi::ToShaderResource(passDataView),
                    .offsetInDrawData = 0
                };

                int vertexOffset = 0;
                int indexOffset = 0;
                ImVec2 clipOff = drawData->DisplayPos;
                int drawDataIdx = 0;

                rg::BufferRegion indexBuffer = ctxt->Resolve(data->indexBuffer);
                for (int n = 0; n < drawData->CmdListsCount; n++)
                {
                    const ImDrawList* drawList = drawData->CmdLists[n];
                    for (int drawIdx = 0; drawIdx < drawList->CmdBuffer.Size; ++drawIdx)
                    {
                        const ImDrawCmd* pcmd = &drawList->CmdBuffer[drawIdx];
                        if (pcmd->UserCallback != nullptr)
                        {
                            if (pcmd->UserCallback == ImDrawCallback_ResetRenderState)
                            {
                                pushConstants.offsetInDrawData = 0;
                            }
                            else
                            {
                                pcmd->UserCallback(drawList, pcmd);
                            }
                        }
                        else
                        {
                            // Apply Scissor, Bind texture, Draw
                            uint32_t x0 = uint32_t(pcmd->ClipRect.x - clipOff.x);
                            uint32_t y0 = uint32_t(pcmd->ClipRect.y - clipOff.y);
                            uint32_t x1 = uint32_t(pcmd->ClipRect.z - clipOff.x);
                            uint32_t y1 = uint32_t(pcmd->ClipRect.w - clipOff.y);
                            cl->SetScissorRect({
                                .x = x0,
                                .y = y0,
                                .width = x1 - x0,
                                .height = y1 - y0
                            });

                            pushConstants.offsetInDrawData = drawDataIdx * sizeof(ImGuiDrawData);
                            cl->PushGraphicsConstants(0, pushConstants);
                            ++drawDataIdx;
                            
                            rhi::IndexedDrawPacket draws[] = {{
                                .pipeline = data->pipeline,
                                .indexBuffer = indexBuffer.buffer,
                                .offsetInIndexBuffer = indexBuffer.offset + (indexOffset + pcmd->IdxOffset) * sizeof(ImDrawIdx),
                                .indexFormat = (sizeof(ImDrawIdx) == sizeof(uint16_t)) ?
                                    rhi::IndexFormat::Uint16 :
                                    rhi::IndexFormat::Uint32,
                                .indexCount = uint32_t(pcmd->ElemCount),
                                .instanceCount = 1
                            }};

                            cl->DrawIndexed(draws);
                        }
                    }
                    indexOffset += drawList->IdxBuffer.Size;
                    vertexOffset += drawList->VtxBuffer.Size;
                }
            }
        }, graphData);

    }

    void ImGuiRenderer::RenderViewportWindows()
    {
        if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
        {
            ImGui::UpdatePlatformWindows();
            ImGui::RenderPlatformWindowsDefault();
        }
    }
}