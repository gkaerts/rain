#ifndef _GLOBALS_H_
#define _GLOBALS_H_

#include "shared/common.h"

static const uint MAX_GLOBAL_UNIFORMS_SIZE = 8 * sizeof(uint);

struct Globals
{
    ShaderResource uniform_FrameData;
    ShaderResource uniform_SceneData;
    ShaderResource uniform_ViewData;
    ShaderResource uniform_PassData;
};
static_assert(sizeof(Globals) <= MAX_GLOBAL_UNIFORMS_SIZE);

#endif // _GLOBALS_H_