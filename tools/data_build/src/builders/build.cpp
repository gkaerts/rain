#include "build.hpp"
#include "toml.hpp"
#include "usd.hpp"

#include "common/memory/memory.hpp"
#include "common/memory/vector.hpp"

#include "flatbuffers/flatbuffers.h"
#include "asset/schema/asset_generated.h"
#include <filesystem>

namespace rn
{
    std::ostream& BuildMessage(std::string_view file)
    {
        return std::cout << "Build: " << file << ": ";
    }

    std::ostream& BuildError(std::string_view file)
    {
        return std::cerr << "ERROR: " << file << ": ";
    }

    std::ostream& BuildWarning(std::string_view file)
    {
        return std::cerr << "WARNING: " << file << ": ";
    }

    std::filesystem::path MakeRelativeTo(std::string_view buildFile, std::string_view otherFile)
    {
        std::filesystem::path buildFilePath = buildFile;
        std::filesystem::path relBuildFileDirectory = buildFilePath.relative_path().parent_path();
        return relBuildFileDirectory / otherFile;
    }

    std::filesystem::path MakeRelativeTo(std::string_view buildFile, std::wstring_view otherFile)
    {
        std::filesystem::path buildFilePath = buildFile;
        std::filesystem::path relBuildFileDirectory = buildFilePath.relative_path().parent_path();
        return relBuildFileDirectory / otherFile;
    }

    int WriteAssetToDisk(std::string_view file, std::string_view extension, const DataBuildOptions& options, Span<const uint8_t> assetData, Span<std::string_view> references, Vector<std::string>& outFiles)
    {
        std::filesystem::path rootDir = options.assetRootDirectory;
        rootDir = std::filesystem::absolute(rootDir);
    
        std::filesystem::path buildFilePath = file;
        buildFilePath = std::filesystem::absolute(buildFilePath);

        std::error_code err;
        buildFilePath = std::filesystem::relative(buildFilePath, rootDir, err);
        if (err)
        {
            BuildError(file) << "Build file is not a descendant of the provided root path \"" << rootDir << "\"" << std::endl;
            return 1;
        }
        
        std::filesystem::path relBuildFileDirectory = buildFilePath.parent_path();
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

        Vector<std::string> sanitizedRefs;
        sanitizedRefs.reserve(references.size());
        for (std::string_view ref : references)
        {
            std::filesystem::path refFilePath = ref;
            refFilePath = std::filesystem::absolute(refFilePath);

            std::error_code err;
            refFilePath = std::filesystem::relative(refFilePath, rootDir, err);
            if (err)
            {
                BuildError(file) << "Dependent asset \"" << refFilePath << "\" is not a descendant of the provided root path \"" << rootDir << "\"" << std::endl;
                return 1;
            }

            sanitizedRefs.push_back(refFilePath.string());
        }
  
        flatbuffers::FlatBufferBuilder fbb;
        auto referenceVec = fbb.CreateVectorOfStrings(sanitizedRefs.begin(), sanitizedRefs.end());
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
        std::filesystem::path rootDir = options.assetRootDirectory;
        rootDir = std::filesystem::absolute(rootDir);
    
        std::filesystem::path buildFilePath = file;
        buildFilePath = std::filesystem::absolute(buildFilePath);

        std::error_code err;
        buildFilePath = std::filesystem::relative(buildFilePath, rootDir, err);
        if (err)
        {
            BuildError(file) << "Build file is not a descendant of the provided root path \"" << rootDir << "\"" << std::endl;
            return 1;
        }
        
        std::filesystem::path relBuildFileDirectory = buildFilePath.parent_path();
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
        std::filesystem::path rootDir = options.assetRootDirectory;
        rootDir = std::filesystem::absolute(rootDir);
    
        std::filesystem::path buildFilePath = file;
        buildFilePath = std::filesystem::absolute(buildFilePath);

        std::error_code err;
        buildFilePath = std::filesystem::relative(buildFilePath, rootDir, err);
        if (err)
        {
            BuildError(file) << "Build file is not a descendant of the provided root path \"" << rootDir << "\"" << std::endl;
            return "error";
        }
        
        std::filesystem::path relBuildFileDirectory = buildFilePath.parent_path();
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
        if (!depArr)
        {
            return true;
        }

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
            if (std::filesystem::exists(depPath))
            {
                std::filesystem::file_time_type lastWriteTime = std::filesystem::last_write_time(depPath);
                std::time_t tt = to_time_t(lastWriteTime);

                toml::table depTable { 
                    { "file"sv, depPath.string() },
                    { "time"sv, tt }
                };

                tomlDeps.push_back(depTable);
            }
            else
            {
                BuildWarning(file) << "\"" << depPath << "\" listed as a build dependency, but file doesn't exist. Incremental builds might break!" << std::endl;
            }
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
        int ret = 0;
        if (!file.empty())
        {
            if (!options.force && !DependenciesChanged(file, options))
            {
                rn::BuildMessage(file) << "File up to date!" << std::endl;
                return 0;
            }

            Vector<std::string> dependencies;
            dependencies.push_back(std::string(options.exeName.data(), options.exeName.size()));
            dependencies.push_back(std::string(file.data(), file.size()));

            if (file.ends_with(".usd") || file.ends_with(".usda") || file.ends_with(".usdc"))
            {
                rn::BuildMessage(file) << "Building USD asset" << std::endl;
                ret = DoBuildUSD(file, options, dependencies);
            }
            else if (file.ends_with(".toml"))
            {
                rn::BuildMessage(file) << "Building TOML asset" << std::endl;
                ret = DoBuildTOML(file, options, dependencies);
            }

            if (ret == 0)
            {
                WriteDependenciesFile(file, options, dependencies);
            }
        }

        return ret;
    }
}