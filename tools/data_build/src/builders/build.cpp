#include "build.hpp"

#include "common/memory/memory.hpp"
#include "common/memory/vector.hpp"

#include "texture.hpp"
#include "material_shader.hpp"
#include "shader.hpp"

#include "flatbuffers/flatbuffers.h"
#include "asset/schema/asset_generated.h"
#include <filesystem>

namespace rn
{
    constexpr const std::pair<std::string_view, FnBuildAsset> BUILDERS[] = 
    {
        { "texture",            BuildTextureAsset },
        { "material_shader",    BuildMaterialShaderAsset },
        { "shader",             BuildShader },
    };

    std::ostream& BuildMessage(std::string_view file)
    {
        return std::cout << "Build: " << file << ": ";
    }

    std::ostream& BuildError(std::string_view file)
    {
        return std::cerr << "ERROR: " << file << ": ";
    }

    std::ostream& BuildError(std::string_view file, const toml::source_region& source)
    {
        return std::cerr << "ERROR: " << file << " (" << source.begin << "): ";
    }

    std::ostream& BuildWarning(std::string_view file)
    {
        return std::cerr << "WARNING: " << file << ": ";
    }

    std::ostream& BuildWarning(std::string_view file, const toml::source_region& source)
    {
        return std::cerr << "WARNING: " << file << " (" << source.begin << "): ";
    }

    bool ValidateTable(std::string_view file, toml::node& node, const TableSchema& schema)
    {
        if (!node.is_table())
        {
            BuildError(file, node.source()) << "Node with name " << schema.name << " expected to be of type 'table' but is of type '" << node.type() << "'" << std::endl;
            return false;
        }

        toml::table& table = *node.as_table();
        for (const Field& key : schema.requiredFields)
        {
            auto field = table[key.name];
            if (!field)
            {
                BuildError(file, table.source()) << "Required field '" << key.name << "' not found in table [" << schema.name << "]" << std::endl;
                return false;
            }
            else if (field.type() != key.type)
            {
                BuildError(file, field.node()->source()) << "Required field '" << key.name << "' in table [" << schema.name << "] is of type '" << field.type() 
                    << "', but expected type '" << key.type << "'" << std::endl;
                return false;
            }

            if (key.fnIsValidValue)
            {
                if (!key.fnIsValidValue(file, *field.node()))
                {
                    BuildError(file, table.source()) << "Required field '" << key.name << "' has an unsupported value of " << field << "" << std::endl;
                    return false;
                }
            }
        }

        for (const Field& key : schema.optionalFields)
        {
            auto field = table[key.name];
            if (field && field.type() != key.type)
            {
                BuildError(file, field.node()->source()) << "Field '" << key.name << "' in table [" << schema.name << "] is of type '" << field.type() 
                    << "', but expected type '" << key.type << "'" << std::endl;
                return false;
            }

            if (field && key.fnIsValidValue)
            {
                if (!key.fnIsValidValue(file, *field.node()))
                {
                    BuildError(file, table.source()) << "Field '" << key.name << "' has an unsupported value of " << field << "" << std::endl;
                    return false;
                }
            }
        }

        return true;
    }

    bool ValidateArray(std::string_view file, std::string_view name, toml::array* arr, toml::node_type elementType)
    {
        if (!arr->empty())
        {
            if (!arr->is_homogeneous(elementType))
            {
                BuildError(file, arr->source()) << "Array '" << name << "' is expected to be homogeneous and of type '" << elementType << "'" << std::endl;
                return false;
            }
        }

        return true;
    }

    int WriteAssetToDisk(std::string_view file, std::string_view extension, const DataBuildOptions& options, Span<const uint8_t> assetData, Span<std::string_view> references, Vector<std::string>& outFiles)
    {
        std::filesystem::path buildFilePath = file;
        std::filesystem::path relBuildFileDirectory = buildFilePath.relative_path().parent_path();
        std::filesystem::path outFilename = buildFilePath.filename();
        outFilename.replace_extension(extension);

        std::filesystem::path outAssetFile = options.outputDirectory / relBuildFileDirectory / outFilename;
        if (!std::filesystem::is_directory(outAssetFile.parent_path()))
        {
            std::filesystem::create_directories(outAssetFile.parent_path());
        }

        FILE* outFile = nullptr;
        fopen_s(&outFile, outAssetFile.string().c_str(), "wb");

        if (!outFile)
        {
            BuildError(file) << "Failed to open file for writing: '" << outAssetFile << "'" << std::endl;
            return 1;
        }

        MemoryScope SCOPE;

        flatbuffers::FlatBufferBuilder fbb;
        ScopedVector<flatbuffers::Offset<flatbuffers::String>> fbReferences(references.size());
        for (std::string_view ref : references)
        {
            fbReferences.push_back(fbb.CreateString(ref));
        }

        auto referenceVec = fbb.CreateVector(fbReferences);
        auto dataVec = fbb.CreateVector(assetData.data(), assetData.size());

        auto fbRoot = schema::CreateAsset(fbb, referenceVec, dataVec);
        fbb.Finish(fbRoot, schema::AssetIdentifier());

        fwrite(fbb.GetBufferPointer(), 1, fbb.GetSize(), outFile);
        fclose(outFile);

        outFiles.push_back(outAssetFile.string());
        return 0;
    }

    int WriteDataToDisk(std::string_view file, std::string_view extension, const DataBuildOptions& options, Span<const uint8_t> data, bool writeText, Vector<std::string>& outFiles)
    {
        std::filesystem::path buildFilePath = file;
        std::filesystem::path relBuildFileDirectory = buildFilePath.relative_path().parent_path();
        std::filesystem::path outFilename = buildFilePath.filename();
        outFilename.replace_extension(extension);

        std::filesystem::path outAssetFile = options.outputDirectory / relBuildFileDirectory / outFilename;

        if (!std::filesystem::is_directory(outAssetFile.parent_path()))
        {
            std::filesystem::create_directories(outAssetFile.parent_path());
        }

        FILE* outFile = nullptr;
        fopen_s(&outFile, outAssetFile.string().c_str(), writeText ? "w" : "wb");

        if (!outFile)
        {
            BuildError(file) << "Failed to open file for writing: '" << outAssetFile << "'" << std::endl;
            return 1;
        }

        fwrite(data.data(), 1, data.size(), outFile);
        fclose(outFile);

        outFiles.push_back(outAssetFile.string());
        return 0;
    }

    const TableSchema BUILD_TABLE_SCHEMA = {
        .name = "build"sv,
        .requiredFields = {
            { .name = "type"sv, .type = toml::node_type::string }
        },
    };

    template <typename TP>
    std::time_t to_time_t(TP tp)
    {
        using namespace std::chrono;
        auto sctp = time_point_cast<system_clock::duration>(tp - TP::clock::now()
                + system_clock::now());
        return system_clock::to_time_t(sctp);
    }

    std::filesystem::path MakeDependenciesPath(std::string_view file, const DataBuildOptions& options)
    {
        std::filesystem::path buildFilePath = file;
        std::filesystem::path relBuildFileDirectory = buildFilePath.relative_path().parent_path();
        std::filesystem::path outFilename = buildFilePath.filename();
        outFilename.replace_extension("toml");

        std::filesystem::path depFile = options.cacheDirectory / relBuildFileDirectory / outFilename;
        return std::filesystem::absolute(depFile);
    }

    bool DependenciesChanged(std::string_view file, const DataBuildOptions& options)
    {
        std::filesystem::path depFile = MakeDependenciesPath(file, options);
        if (!std::filesystem::exists(depFile))
        {
            return true;
        }

        auto root = toml::parse_file(depFile.c_str());
        auto dependencies = root["dependencies"];

        toml::array* depArr = dependencies.as_array();
        for (auto& el : *depArr)
        {
            auto& table = *el.as_table();
            std::filesystem::path path = table["file"].value_or(""sv);
            std::time_t writeTime = table["time"].value_or<std::time_t>(0);

            if (!std::filesystem::exists(path))
            {
                return true;
            }

            std::filesystem::file_time_type lastWriteTime = std::filesystem::last_write_time(path);
            std::time_t tt = to_time_t(lastWriteTime);
            if (writeTime != tt)
            {
                return true;
            }
        }

        return false;
    }

    void WriteDependenciesFile(std::string_view file, const DataBuildOptions& options, Span<const std::string> dependencies)
    {
        std::filesystem::path outDepFile = MakeDependenciesPath(file, options);

        auto tomlDeps = toml::array();
        for (const std::string& str : dependencies)
        {
            std::filesystem::path depPath = std::filesystem::absolute(str);
            std::filesystem::file_time_type lastWriteTime = std::filesystem::last_write_time(depPath);
            std::time_t tt = to_time_t(lastWriteTime);

            toml::table depTable { 
                { "file"sv, depPath.string() },
                { "time"sv, tt }
            };

            tomlDeps.push_back(depTable);
        }

        auto root = toml::table();
        root.insert_or_assign("dependencies", tomlDeps);

        if (!std::filesystem::is_directory(outDepFile.parent_path()))
        {
            std::filesystem::create_directories(outDepFile.parent_path());
        }

        FILE* outFile = nullptr;
        fopen_s(&outFile, outDepFile.string().c_str(), "w");

        if (!outFile)
        {
            BuildError(file) << "Failed to open file for writing: '" << outDepFile << "'" << std::endl;
            return;
        }

        std::stringstream stream;
        stream << root;

        std::string data = stream.str();
        
        fwrite(data.data(), 1, data.length(), outFile);
        fclose(outFile);

        return;
    }

    int DoBuild(std::string_view file, const DataBuildOptions& options)
    {
        if (!file.empty())
        {
            if (!DependenciesChanged(file, options))
            {
                rn::BuildMessage(file) << "File up to date!" << std::endl;
                return 0;
            }

            try
            {
                rn::BuildMessage(file) << "Building asset" << std::endl;
                auto root = toml::parse_file(file);

                auto build = root["build"];
                if (!build)
                {
                    rn::BuildError(file) << "No [build] table found" << std::endl;
                    return 1;
                }

                if (!ValidateTable(file, *build.node(), BUILD_TABLE_SCHEMA))
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
                    rn::BuildError(file) << "Unknown build type specified: " << buildType << std::endl;
                    return 1;
                }

                Vector<std::string> dependencies;
                dependencies.push_back(std::string(options.exeName.data(), options.exeName.size()));
                dependencies.push_back(std::string(file.data(), file.size()));

                int ret = onBuildAsset(file, root, options, dependencies);
                if (ret == 0)
                {
                    WriteDependenciesFile(file, options, dependencies);
                }

                return ret;
            }
            catch(const toml::parse_error& e)
            {
                rn::BuildError(file) << "Failed to parse TOML file:" << std::endl;
                std::cerr << e << std::endl;
                return 1;
            }
        }

        return 0;
    }
}