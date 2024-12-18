#include "common/common.hpp"
#include "app/application.hpp"
#include "app/render_window.hpp"

#include "rhi/swap_chain.hpp"
#include "rhi/device.hpp"
#include "rhi/command_list.hpp"

#include "rg/render_graph.hpp"

#include "render/systems/imgui/imgui.hpp"
#include "backends/imgui_impl_sdl2.h"

#include "imgui.h"

using namespace rn;

void EventListenerHook(void* event)
{
    ImGui_ImplSDL2_ProcessEvent(static_cast<SDL_Event*>(event));
}

int main(int argc, char* argv[])
{
    app::ApplicationConfig config = {
        .threadScopeBackingSize = 16 * MEGA,
        .loggerSettings = {
            .desktop = {
                .logFilename = "log_game.txt"
            }
        },
        .rhiDeviceType = app::RHIDeviceType::D3D12,
        .mainWindowWidth = 1280,
        .mainWindowHeight = 720,
        .eventListenerHook = EventListenerHook,
        .argc = argc,
        .argv = argv
    };

    app::Application app(config);

    rhi::CommandList* uploadCL = app.RHIDevice()->AllocateCommandList();
    render::ImGuiRenderer imguiRenderer(app.RHIDevice(), uploadCL, {
        .outputFormat = rhi::RenderTargetFormat::RGBA8Unorm,
        .window = app.MainWindow()->SDLWindow()
    });

    app.RHIDevice()->SubmitCommandLists({ &uploadCL, 1 });
    
    rg::RenderGraph renderGraph(app.RHIDevice());

    while (true)
    {
        if (!app.ProcessEvents())
        {
            break;
        }

        app::RenderWindow* window = app.MainWindow();
        if (window->IsVisible())
        {
            float DPI = window->DPIScale();
            imguiRenderer.BeginFrame(DPI);
            ImGui::ShowDemoWindow();

            rhi::Device* device = app.RHIDevice();
            rhi::SwapChain* swapChain = window->RHISwapChain();

            swapChain->WaitForRender();
            

            uint2 drawableSize = window->DrawableSize();
            renderGraph.Reset({
                .x = 0,
                .y = 0,
                .width = drawableSize.x,
                .height = drawableSize.y
            });

            rhi::BackBuffer currentBackBuffer = swapChain->CurrentBackBuffer();
            rg::Texture2D rgBackBuffer = renderGraph.RegisterTexture2D({
                .texture = currentBackBuffer.texture,
                .rtv = currentBackBuffer.view,
                .clearValue = rhi::ClearColor(1.0f, 0.0f, 0.0f, 1.0f),
                .lastSyncStage = rhi::PipelineSyncStage::None,
                .lastAccess = rhi::PipelineAccess::None,
                .lastLayout = rhi::TextureLayout::Present,
                .name = "Swap chain back buffer"
            });

            renderGraph.AddRenderPass<void>({
                .name = "Clear back buffer",
                .flags = rg::RenderPassFlags::IsSmall,
                .colorAttachments = {
                    { .texture = rgBackBuffer, .loadOp = rhi::LoadOp::Clear }
                }
            }, nullptr);

            imguiRenderer.Render(&renderGraph, {
                .output = rgBackBuffer
            });

            renderGraph.AddRenderPass<void>({
                .name = "Make back buffer presentable",
                .flags = rg::RenderPassFlags::IsSmall,
                .texture2Ds = {
                    rg::Present(rgBackBuffer)
                }
            }, nullptr);

            renderGraph.Build();
            renderGraph.Execute(rg::RenderGraphExecutionFlags::ForceSingleThreaded);

            device->EndFrame();
            swapChain->Present();

            imguiRenderer.RenderViewportWindows();
        }
    }

    return 0;
}