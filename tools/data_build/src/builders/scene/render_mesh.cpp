#pragma warning(push, 1)
#include "rn/renderMeshComponentAPI.h"
#include "rn/renderMaterialListComponentAPI.h"
#pragma warning(pop)

#include "builders/build.hpp"
#include "builders/usd.hpp"
#include "builders/scene/scene.hpp"

#include "mesh_gen.hpp"

// bool BuildRenderMeshComponent(std::string_view file, const pxr::RnRenderMeshComponentAPI& usdMesh, BumpAllocator& blobAllocator, render::schema::RenderMesh& outMesh)
// {}