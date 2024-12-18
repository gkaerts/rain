#ifndef _GLOBALS_HLSLI_
#define _GLOBALS_HLSLI_

#include "shared/globals.h"


#if _D3D12_
    #define PushConstants(type, name) ConstantBuffer<type> name : register(b0);

#elif _VULKAN_
    #define PushConstants(type, name) [[vk::push_constants]] type name;

#endif

PushConstants(Globals, _Globals)

#define LoadFrameData() ResourceDescriptorHeap[_Globals.uniform_FrameData]
#define LoadSceneData() ResourceDescriptorHeap[_Globals.uniform_SceneData]
#define LoadViewData()  ResourceDescriptorHeap[_Globals.uniform_ViewData]
#define LoadPassData()  ResourceDescriptorHeap[_Globals.uniform_PassData]

#endif // _GLOBALS_HLSLI_