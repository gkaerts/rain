#ifndef _GLOBALS_HLSLI_
#define _GLOBALS_HLSLI_

#include "shared/globals.h"

#if _D3D12_
    ConstantBuffer<Globals> _Globals : register(b0);

#elif _VULKAN_
    [[vk::push_constants]] Globals _Globals;

#endif

#endif // _GLOBALS_HLSLI_