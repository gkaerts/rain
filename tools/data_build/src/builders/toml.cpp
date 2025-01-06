#define TOML_IMPLEMENTATION
#include "toml++/toml.hpp"

#include "toml.hpp"
#include "shader.hpp"

namespace rn
{
    namespace
    {
        constexpr const std::pair<std::string_view, FnBuildAsset> BUILDERS[] = 
        {
            { "shader", BuildShader },
        };

        const TableSchema BUILD_TABLE_SCHEMA = {
            .name = "build"sv,
            .requiredFields = {
                { .name = "type"sv, .type = toml::node_type::string }
            },
        };
    }

    std::ostream& BuildError(const DataBuildContext& ctxt, const toml::source_region& source)
    {
        return std::cerr << "ERROR: " << ctxt.file << " (" << source.begin << "): ";
    }

    std::ostream& BuildWarning(const DataBuildContext& ctxt, const toml::source_region& source)
    {
        return std::cerr << "WARNING: " << ctxt.file << " (" << source.begin << "): ";
    }

    bool ValidateTable(const DataBuildContext& ctxt, toml::node& node, const TableSchema& schema)
    {
        if (!node.is_table())
        {
            BuildError(ctxt, node.source()) << "Node with name " << schema.name << " expected to be of type 'table' but is of type '" << node.type() << "'" << std::endl;
            return false;
        }

        toml::table& table = *node.as_table();
        for (const Field& key : schema.requiredFields)
        {
            auto field = table[key.name];
            if (!field)
            {
                BuildError(ctxt, table.source()) << "Required field '" << key.name << "' not found in table [" << schema.name << "]" << std::endl;
                return false;
            }
            else if (field.type() != key.type)
            {
                BuildError(ctxt, field.node()->source()) << "Required field '" << key.name << "' in table [" << schema.name << "] is of type '" << field.type() 
                    << "', but expected type '" << key.type << "'" << std::endl;
                return false;
            }

            if (key.fnIsValidValue)
            {
                if (!key.fnIsValidValue(ctxt, *field.node()))
                {
                    BuildError(ctxt, table.source()) << "Required field '" << key.name << "' has an unsupported value of " << field << "" << std::endl;
                    return false;
                }
            }
        }

        for (const Field& key : schema.optionalFields)
        {
            auto field = table[key.name];
            if (field && field.type() != key.type)
            {
                BuildError(ctxt, field.node()->source()) << "Field '" << key.name << "' in table [" << schema.name << "] is of type '" << field.type() 
                    << "', but expected type '" << key.type << "'" << std::endl;
                return false;
            }

            if (field && key.fnIsValidValue)
            {
                if (!key.fnIsValidValue(ctxt, *field.node()))
                {
                    BuildError(ctxt, table.source()) << "Field '" << key.name << "' has an unsupported value of " << field << "" << std::endl;
                    return false;
                }
            }
        }

        return true;
    }

    bool ValidateArray(const DataBuildContext& ctxt, std::string_view name, toml::array* arr, toml::node_type elementType)
    {
        if (!arr->empty())
        {
            if (!arr->is_homogeneous(elementType))
            {
                BuildError(ctxt, arr->source()) << "Array '" << name << "' is expected to be homogeneous and of type '" << elementType << "'" << std::endl;
                return false;
            }
        }

        return true;
    }

    int DoBuildTOML(const DataBuildContext& ctxt, Vector<std::string>& outFiles)
    {
        int ret = 0;
        try
        {
            rn::BuildMessage(ctxt) << "Building asset" << std::endl;
            auto root = toml::parse_file(ctxt.file);

            auto build = root["build"];
            if (!build)
            {
                rn::BuildError(ctxt) << "No [build] table found" << std::endl;
                return 1;
            }

            if (!ValidateTable(ctxt, *build.node(), BUILD_TABLE_SCHEMA))
            {
                return 1;
            }

            std::string_view buildType = build["type"].value_or(""sv);
            FnBuildAsset onBuildAsset = nullptr;
            for (const auto& builder : BUILDERS)
            {
                if (builder.first == buildType)
                {
                    onBuildAsset = builder.second;
                    break;
                }
            }

            if (!onBuildAsset)
            {
                rn::BuildError(ctxt) << "Unknown build type specified: " << buildType << std::endl;
                return 1;
            }

            ret = onBuildAsset(ctxt, root, outFiles);
        }
        catch(const toml::parse_error& e)
        {
            rn::BuildError(ctxt) << "Failed to parse TOML file:" << std::endl;
            std::cerr << e << std::endl;
            return 1;
        }

        return ret;
    }
}