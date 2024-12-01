#pragma once

#include "build.hpp"
#include "toml.hpp"
#include "common/memory/vector.hpp"

#include "toml++/toml.hpp"
#include <string_view>

#include <Windows.h>
#include "dxcapi.h"
#include <wrl.h>

#include <string_view>
#include <string>

namespace rn
{
    struct DataBuildOptions;
    int BuildShader(std::string_view file, toml::parse_result& root, const DataBuildOptions& options, Vector<std::string>& outFiles);

    enum class ShaderTarget : uint32_t
    {
        Vertex,
        Pixel,
        Amplification,
        Mesh,
        Compute,
        Library,
        Count
    };

    enum class TargetAPI : uint32_t
    {
        D3D12 = 0,
        //Vulkan = 1,
        Count
    };

    struct EntryPoint
    {
        ShaderTarget target;
        std::wstring name;
    };
    struct ShaderCompilationResult
    {
        uint32_t entryPointIdx;
        Microsoft::WRL::ComPtr<IDxcResult> result;
    };

    bool CompileShadersForTargetAPI(std::string_view file, 
        const DataBuildOptions& options,
        TargetAPI api,
        const std::wstring& sourceFile,
        Span<const EntryPoint> entryPoints, 
        Span<const std::wstring> defines,
        Span<const std::wstring> includeDirs,
        ScopedVector<ShaderCompilationResult>& results,
        Vector<std::string>& outFiles);
}