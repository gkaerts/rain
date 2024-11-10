#include "rhi/resource.hpp"

namespace rn::rhi
{
    ClearValue ClearColor(float r, float g, float b, float a)
    {
        return {
            .color = { r, g, b, a }
        };
    }

    ClearValue ClearDepthStencil(float depth, uint8_t stencil)
    {
        return {
            .depthStencil = {
                .depth = depth,
                .stencil = stencil
            }
        };
    }
}