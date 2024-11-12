#include "common/common.hpp"
#include "app/application.hpp"
#include "app/render_window.hpp"
#include "rhi/swap_chain.hpp"
#include "rhi/device.hpp"
#include "rhi/command_list.hpp"

using namespace rn;
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
        .argc = argc,
        .argv = argv
    };

    app::Application app(config);
    

    while (true)
    {
        if (!app.ProcessEvents())
        {
            break;
        }

        app::RenderWindow* window = app.MainWindow();
        if (window->IsVisible())
        {
            rhi::Device* device = app.RHIDevice();
            rhi::SwapChain* swapChain = window->RHISwapChain();

            swapChain->WaitForRender();

            uint2 drawableSize = window->DrawableSize();
            rhi::CommandList* cl = device->AllocateCommandList();

            cl->Barrier({
                .texture2DBarriers = {{
                    .fromStage = rhi::PipelineSyncStage::None,
                    .toStage = rhi::PipelineSyncStage::RenderTargetOutput,
                    .fromAccess = rhi::PipelineAccess::NoAccess,
                    .toAccess = rhi::PipelineAccess::RenderTargetWrite,
                    .fromLayout = rhi::TextureLayout::Present,
                    .toLayout = rhi::TextureLayout::RenderTarget,
                    .handle = swapChain->CurrentBackBuffer().texture,
                    .numMips = 1,
                    .numArraySlices = 1
                }}
            });

            cl->BeginRenderPass({
                .viewport = { 0, 0, drawableSize.x, drawableSize.y },

                .renderTargets = {{
                    .view = swapChain->CurrentBackBuffer().view,
                    .loadOp = rhi::LoadOp::Clear,
                    .clearValue = rhi::ClearColor(1.0f, 0.0f, 0.0f, 1.0f)
                }},

                .depthTarget = {
                    .view = rhi::DepthStencilView::Invalid
                }
            });


            cl->EndRenderPass();

            cl->Barrier({
                .texture2DBarriers = {{
                    .fromStage = rhi::PipelineSyncStage::RenderTargetOutput,
                    .toStage = rhi::PipelineSyncStage::None,
                    .fromAccess = rhi::PipelineAccess::RenderTargetWrite,
                    .toAccess = rhi::PipelineAccess::NoAccess,
                    .fromLayout = rhi::TextureLayout::RenderTarget,
                    .toLayout = rhi::TextureLayout::Present,
                    .handle = swapChain->CurrentBackBuffer().texture,
                    .numMips = 1,
                    .numArraySlices = 1
                }}
            });

            device->SubmitCommandLists({&cl, 1});

            device->EndFrame();
            swapChain->Present();
        }
    }

    return 0;
}