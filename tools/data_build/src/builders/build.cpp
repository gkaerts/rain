#include "build.hpp"
#include "toml.hpp"
#include "usd.hpp"

#include "common/memory/memory.hpp"
#include "common/memory/vector.hpp"

#include "asset_gen.hpp"
#include "luagen/schema.hpp"
#include <filesystem>

namespace rn
{
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

    std::filesystem::path MakeRootRelativeReferencePath(const DataBuildContext& ctxt, std::string_view refPath)
    {
        std::filesystem::path rootDir = ctxt.options.assetRootDirectory;
        rootDir = std::filesystem::absolute(rootDir);

        std::filesystem::path refFilePath = refPath;
        refFilePath = std::filesystem::absolute(refFilePath);

        std::error_code err;
        refFilePath = std::filesystem::relative(refFilePath, rootDir, err);
        if (err)
        {
            BuildError(ctxt) << "Dependent asset \"" << refFilePath << "\" is not a descendant of the provided root path \"" << rootDir << "\"" << std::endl;
            return "";
        }

        return refFilePath;
    }

    std::ostream& BuildMessage(const DataBuildContext& ctxt)
    {
        return std::cout << "Build: " << ctxt.file << ": ";
    }

    std::ostream& BuildError(const DataBuildContext& ctxt)
    {
        return std::cerr << "ERROR: " << ctxt.file << ": ";
    }

    std::ostream& BuildWarning(const DataBuildContext& ctxt)
    {
        return std::cerr << "WARNING: " << ctxt.file << ": ";
    }

    int WriteAssetToDisk(const DataBuildContext& ctxt, std::string_view extension, Span<uint8_t> assetData, Span<std::string_view> references, Vector<std::string>& outFiles)
    {
        std::string_view file = ctxt.file;
        const DataBuildOptions& options = ctxt.options;

        std::filesystem::path rootDir = options.assetRootDirectory;
        rootDir = std::filesystem::absolute(rootDir);
    
        std::filesystem::path buildFilePath = file;
        buildFilePath = std::filesystem::absolute(buildFilePath);

        std::error_code err;
        buildFilePath = std::filesystem::relative(buildFilePath, rootDir, err);
        if (err)
        {
            BuildError(ctxt) << "Build file is not a descendant of the provided root path \"" << rootDir << "\"" << std::endl;
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
            BuildError(ctxt) << "Failed to open file for writing: '" << outAssetFile << "'" << std::endl;
            return 1;
        }

        Vector<std::string> sanitizedRefs;
        sanitizedRefs.reserve(references.size());
        for (std::string_view ref : references)
        {
            std::filesystem::path refFilePath = MakeRootRelativeReferencePath(ctxt, ref);
            if (refFilePath.empty())
            {
                return 1;
            }

            sanitizedRefs.push_back(refFilePath.string());
        }

        MemoryScope SCOPE;

        using namespace asset;
        schema::Asset outAsset = {
            .identifier = extension,
            .references = { ScopedNewArray<std::string_view>(SCOPE, sanitizedRefs.size()), sanitizedRefs.size() },
            .assetData = assetData
        };

        for (int refIdx = 0; const std::string& sanitizedRef : sanitizedRefs)
        {
            outAsset.references[refIdx++] = sanitizedRef;
        }

        size_t serializedSize = schema::Asset::SerializedSize(outAsset);
        Span<uint8_t> outData = { static_cast<uint8_t*>(ScopedAlloc(serializedSize, CACHE_LINE_TARGET_SIZE)), serializedSize };
        rn::Serialize<schema::Asset>(outData, outAsset);

        fwrite(outData.data(), 1, outData.size(), outFile);
        fclose(outFile);

        outFiles.push_back(outAssetFile.string());
        return 0;
    }

    int WriteDataToDisk(const DataBuildContext& ctxt, std::string_view extension, Span<uint8_t> data, bool writeText, Vector<std::string>& outFiles)
    {
        std::string_view file = ctxt.file;
        const DataBuildOptions& options = ctxt.options;

        std::filesystem::path rootDir = options.assetRootDirectory;
        rootDir = std::filesystem::absolute(rootDir);
    
        std::filesystem::path buildFilePath = file;
        buildFilePath = std::filesystem::absolute(buildFilePath);

        std::error_code err;
        buildFilePath = std::filesystem::relative(buildFilePath, rootDir, err);
        if (err)
        {
            BuildError(ctxt) << "Build file is not a descendant of the provided root path \"" << rootDir << "\"" << std::endl;
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
            BuildError(ctxt) << "Failed to open file for writing: '" << outAssetFile << "'" << std::endl;
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

    std::filesystem::path MakeDependenciesPath(const DataBuildContext& ctxt)
    {
        std::string_view file = ctxt.file;
        const DataBuildOptions& options = ctxt.options;

        std::filesystem::path rootDir = options.assetRootDirectory;
        rootDir = std::filesystem::absolute(rootDir);
    
        std::filesystem::path buildFilePath = file;
        buildFilePath = std::filesystem::absolute(buildFilePath);

        std::error_code err;
        buildFilePath = std::filesystem::relative(buildFilePath, rootDir, err);
        if (err)
        {
            BuildError(ctxt) << "Build file is not a descendant of the provided root path \"" << rootDir << "\"" << std::endl;
            return "error";
        }
        
        std::filesystem::path relBuildFileDirectory = buildFilePath.parent_path();
        std::filesystem::path outFilename = buildFilePath.filename();
        outFilename.replace_extension("toml");

        std::filesystem::path depFile = options.cacheDirectory / relBuildFileDirectory / outFilename;
        return std::filesystem::absolute(depFile);
    }

    bool DependenciesChanged(const DataBuildContext& ctxt)
    {
        std::filesystem::path depFile = MakeDependenciesPath(ctxt);
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

    void WriteDependenciesFile(const DataBuildContext& ctxt, Span<const std::string> dependencies)
    {
        std::filesystem::path outDepFile = MakeDependenciesPath(ctxt);

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
                BuildWarning(ctxt) << "\"" << depPath << "\" listed as a build dependency, but file doesn't exist. Incremental builds might break!" << std::endl;
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
            BuildError(ctxt) << "Failed to open file for writing: '" << outDepFile << "'" << std::endl;
            return;
        }

        std::stringstream stream;
        stream << root;

        std::string data = stream.str();
        
        fwrite(data.data(), 1, data.length(), outFile);
        fclose(outFile);

        return;
    }

    int DoBuild(const DataBuildContext& ctxt)
    {
        std::string_view file = ctxt.file;
        const DataBuildOptions& options = ctxt.options;

        int ret = 0;
        if (!file.empty())
        {
            if (!ctxt.options.force && !DependenciesChanged(ctxt))
            {
                rn::BuildMessage(ctxt) << "File up to date!" << std::endl;
                return 0;
            }

            Vector<std::string> dependencies;
            dependencies.push_back(std::string(options.exeName.data(), options.exeName.size()));
            dependencies.push_back(std::string(file.data(), file.size()));

            if (file.ends_with(".usd") || file.ends_with(".usda") || file.ends_with(".usdc"))
            {
                rn::BuildMessage(ctxt) << "Building USD asset" << std::endl;
                ret = DoBuildUSD(ctxt, dependencies);
            }
            else if (file.ends_with(".toml"))
            {
                rn::BuildMessage(ctxt) << "Building TOML asset" << std::endl;
                ret = DoBuildTOML(ctxt, dependencies);
            }

            if (ret == 0)
            {
                WriteDependenciesFile(ctxt, dependencies);
            }
        }

        return ret;
    }
}