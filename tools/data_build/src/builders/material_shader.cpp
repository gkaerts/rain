#include "build.hpp"
#include "common/memory/vector.hpp"
#include "common/memory/span.hpp"

namespace rn
{
    struct RasterPass
    {
        std::string_view name;
        std::string_view vertexShader;
        std::string_view pixelShader;
    };

    struct RayTracingPass
    {
        std::string_view name;
        std::string_view hitGroupName;
        std::string_view closestHitShader;
        std::string_view anyHitShader;
    };

    struct Parameter
    {
        std::string_view temp;
    };

    struct ParameterGroup
    {
        std::string_view name;
        Vector<Parameter> parameters;
    };

    struct MaterialShader
    {
        std::string_view source;
        Vector<std::string_view> defines;

        Vector<RasterPass> rasterPasses;
        Vector<RayTracingPass> rayTracingPasses;
        Vector<ParameterGroup> parameterGroups;
    };

    const TableSchema MATERIAL_SHADER_TABLE_SCHEMA = {
        .name = "material_shader"sv,
        .requiredFields = {
            { .name = "source"sv, .type = toml::node_type::string }
        },

        .optionalFields = {
            { .name = "defines"sv, .type = toml::node_type::array }
        }
    };

    const TableSchema RASTER_PASS_TABLE_SCHEMA = {
        .name = "raster_pass"sv,
        .requiredFields = {
            { .name = "name"sv, .type = toml::node_type::string },
            { .name = "vertex_shader"sv, .type = toml::node_type::string },
        },

        .optionalFields = {
            { .name = "pixel_shader"sv, .type = toml::node_type::string },
        }
    };

    const TableSchema RAY_TRACING_PASS_TABLE_SCHEMA = {
        .name = "ray_tracing_pass"sv,
        .requiredFields = {
            { .name = "name"sv, .type = toml::node_type::string },
            { .name = "hit_group_name"sv, .type = toml::node_type::string },
            { .name = "closest_hit_shader"sv, .type = toml::node_type::string },
        },

        .optionalFields = {
            { .name = "any_hit_shader"sv, .type = toml::node_type::string },
        }
    };

    bool ParseMaterialShader(MaterialShader& outShader, std::string_view file, toml::parse_result& root, Vector<std::string>& outFiles)
    {
        if (!root["material_shader"])
        {
            BuildError(file) << "No [material_shader] table found" << std::endl;
            return false;
        }

        auto materialShader = root["material_shader"];
        if (!ValidateTable(file, *materialShader.node(), MATERIAL_SHADER_TABLE_SCHEMA))
        {
            return false;
        }

        outShader.source = materialShader["source"].value_or(""sv);

        if (toml::array* defines = materialShader["defines"].as_array())
        {
            if (!ValidateArray(file, "defines", defines, toml::node_type::string))
            {
                return false;
            }

            for (auto& define : *defines)
            {
                outShader.defines.push_back(define.value_or(""sv));
            }
        }

        outFiles.push_back(std::string(outShader.source.data(), outShader.source.size()));
        return true;
    }

    bool ParseRasterPasses(MaterialShader& outShader, std::string_view file, toml::parse_result& root)
    {
        auto rasterPasses = root["raster_pass"];
        if (toml::array* passes = rasterPasses.as_array())
        {
            if (!ValidateArray(file, "raster_pass", passes, toml::node_type::table))
            {
                return false;
            }

            for (auto& el : *passes)
            {
                if (!ValidateTable(file, el, RASTER_PASS_TABLE_SCHEMA))
                {
                    return false;
                }

                toml::table& pass = *el.as_table();
                outShader.rasterPasses.push_back({
                    .name =         pass["name"].value_or(""sv),
                    .vertexShader = pass["vertex_shader"].value_or(""sv),
                    .pixelShader =  pass["pixel_shader"].value_or(""sv)
                });
            }
        }

        return true;
    }

    bool ParseRayTracingPasses(MaterialShader& outShader, std::string_view file, toml::parse_result& root)
    {
        auto rtPasses = root["ray_tracing_pass"];
        if (toml::array* passes = rtPasses.as_array())
        {
            if (!ValidateArray(file, "ray_tracing_pass", passes, toml::node_type::table))
            {
                return false;
            }

            for (auto& el : *passes)
            {
                if (!ValidateTable(file, el, RAY_TRACING_PASS_TABLE_SCHEMA))
                {
                    return false;
                }   

                toml::table& pass = *el.as_table();
                outShader.rayTracingPasses.push_back({
                    .name =             pass["name"].value_or(""sv),
                    .hitGroupName =     pass["hit_group_name"].value_or(""sv),
                    .closestHitShader = pass["closest_hit_shader"].value_or(""sv),
                    .anyHitShader =     pass["any_hit_shader"].value_or(""sv)
                });
            }
        }

        return true;
    }

    int BuildMaterialShaderAsset(std::string_view file, toml::parse_result& root, const DataBuildOptions& options, Vector<std::string>& outFiles)
    {
        MaterialShader shader = {};
        if (!ParseMaterialShader(shader, file, root, outFiles) ||
            !ParseRasterPasses(shader, file, root) || 
            !ParseRayTracingPasses(shader, file, root))
        {
            return 1;
        }

        return 0;
    }
}