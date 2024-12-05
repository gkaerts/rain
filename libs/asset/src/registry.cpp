#include "asset/registry.hpp"
#include "common/memory/string.hpp"
#include "common/log/log.hpp"
#include "mio/mio.hpp"

#include "common/task/scheduler.hpp"
#include "TaskScheduler.h"

#include "asset_gen.hpp"
#include "luagen/schema.hpp"


namespace rn::asset
{
    RN_DEFINE_LOG_CATEGORY(Asset);

    namespace
    {
        const std::string_view PathExtension(const std::string_view& path)
        {
            size_t extOffset = path.find_last_of('.');
            if (extOffset == std::string_view::npos)
            {
                return {};
            }

            return path.substr(extOffset);
        }

        void SanitizePath(String& path, bool maintainTrailingSlash = false)
        {
            // Forward slashes only
            std::replace(path.begin(), path.end(), '\\', '/');

            // All lower case
            std::transform(path.begin(), path.end(), path.begin(), [](char c)
            {
                return std::tolower(c);
            });

            // No trailing slashes
            if (!maintainTrailingSlash && path.ends_with("/"))
            {
                path.pop_back();
            }
        }

        class MappedFileAsset : public MappedAsset
        {
        public:
            MappedFileAsset(const String& path)
            {
                std::error_code err;
                _source.map(path.c_str(), err);

                RN_ASSERT(err.value() == 0);
            }

            virtual Span<const uint8_t> Ptr() const override { return { reinterpret_cast<const uint8_t*>(_source.data()), _source.length() }; }

            mio::mmap_source _source;
        };
    }

    MappedAsset* MapFileAsset(MemoryScope& scope, const String& path)
    {
        return ScopedNew<MappedFileAsset>(scope, path);
    }

    Registry::Registry(const RegistryDesc& desc)
        : _contentPrefix(desc.contentPrefix)
        , _enableMultithreadedLoad(desc.enableMultithreadedLoad)
        , _onMapAsset(desc.onMapAsset)
    {
        SanitizePath(_contentPrefix, true);
    }

    Registry::~Registry()
    {
        for (const auto& it : _extensionHashToBank)
        {
            TrackedDelete(it.second);
        }

        _extensionHashToBank.clear();
        _handleIDToBank.clear();
    }

    Asset Registry::LoadInternal(std::string_view identifier, LoadFlags flags)
    {
        MemoryScope SCOPE;
        String path = { identifier.data(), identifier.size() };

        SanitizePath(path);
        const std::string_view ext = PathExtension(path);

        // Invalid identifier
        RN_ASSERT(!ext.empty());
        StringHash extHash = HashString(ext);

        auto bankIt = _extensionHashToBank.find(extHash);

        // Did you forget to register this asset type?
        RN_ASSERT(bankIt != _extensionHashToBank.end());

        StringHash identifierHash = HashString(path);
        BankBase* bank = bankIt->second;

        bool doReload = TestFlag(flags, LoadFlags::Reload);
        std::pair<bool, Asset> handle = bank->FindOrAllocateHandle(identifierHash);
        if (handle.first || (!handle.first && doReload))
        {
            // We're either loading a new asset (i.e. it didn't exist before) or we're forcibly reloading an asset
            LogInfo(LogCategory::Asset, "{} asset \"{}\" at handle 0x{:x}",
                doReload ? "Reloading" : "Loading",
                path.c_str(),
                size_t(handle.second));

            String fullPath = _contentPrefix;
            fullPath.append(path);

            MappedAsset* mapping = _onMapAsset(SCOPE, fullPath);

            // File is not a valid asset file
            
            auto fnAlloc = [](size_t size) { return ScopedAlloc(size, CACHE_LINE_TARGET_SIZE); };

            const schema::Asset asset = rn::Deserialize<schema::Asset>(mapping->Ptr(), fnAlloc);

            if (_enableMultithreadedLoad)
            {
                enki::TaskScheduler* scheduler = TaskScheduler();

                struct HandleAssetLoadTask : enki::ITaskSet
                {
                    Registry* registry = nullptr;
                    BankBase* bank = nullptr;
                    const schema::Asset* asset = nullptr;
                    Asset destHandle = Asset::Invalid;
                    std::string_view identifier;
                    String path;

                    Span<Asset> dependencies;
                    bool doReschedule = false;

                    void ExecuteRange(enki::TaskSetPartition range_, uint32_t threadnum_) override 
                    {
                        bool allDependenciesResident = true;
                        uint32_t dependencyIdx = 0;
                        for (Asset dependency : dependencies)
                        {
                            std::string_view ext = PathExtension(asset->references[dependencyIdx]);
                            StringHash extHash = HashString(ext);

                            auto bankIt = registry->_extensionHashToBank.find(extHash);
                            RN_ASSERT(bankIt != registry->_extensionHashToBank.end());

                            BankBase* dependencyBank = bankIt->second;
                            if (dependencyBank->AssetResidency(dependency) != Residency::Resident)
                            {
                                LogInfo(LogCategory::Asset, "Asset \"{}\" is waiting on dependency {} ({})", 
                                    path.c_str(), 
                                    dependencyIdx,
                                    asset->references[dependencyIdx]);
                                allDependenciesResident = false;
                                break;
                            }
                            ++dependencyIdx;
                        }

                        if (allDependenciesResident)
                        {
                            bank->Store(destHandle, {
                                .identifier = identifier,
                                .data = asset->assetData,
                                .dependencies = dependencies,
                                .registry = registry
                            });
                        }
                        else
                        {
                            // Reschedule if our dependencies aren't ready yet
                            doReschedule = true;
                        }
                    }
                };

                struct ResolveAssetDependencyTask : enki::ITaskSet
                {
                    Registry* registry;
                    String referencePath;
                    LoadFlags flags;
                    Asset* destHandle;

                    void ExecuteRange(enki::TaskSetPartition range_, uint32_t threadnum_) override
                    {
                        *destHandle = registry->LoadInternal(referencePath, flags);
                    }
                };

                size_t referenceCount = asset.references.size();
                ScopedVector<enki::Dependency> taskDependencies(referenceCount);
                ScopedVector<ResolveAssetDependencyTask> referenceTasks(referenceCount);
                ScopedVector<Asset> dependentHandles(referenceCount);
                ScopedVector<String> sanitizedReferenceStrings(referenceCount);

                HandleAssetLoadTask handleLoadTask;
                handleLoadTask.registry = this;
                handleLoadTask.bank = bank;
                handleLoadTask.asset = &asset;
                handleLoadTask.destHandle = handle.second;
                handleLoadTask.identifier = identifier;
                handleLoadTask.path = path;
                handleLoadTask.dependencies = dependentHandles;

                if (!asset.references.empty())
                {
                    size_t dependencyIdx = 0;
                    for (const std::string_view reference : asset.references)
                    {
                        String& referencePath = sanitizedReferenceStrings[dependencyIdx];
                        referencePath = { reference.data(), reference.length() };
                        SanitizePath(referencePath);

                        ResolveAssetDependencyTask& dependencyTask = referenceTasks[dependencyIdx];
                        dependencyTask.registry = this;
                        dependencyTask.referencePath = referencePath;
                        dependencyTask.flags = flags;
                        dependencyTask.destHandle = &dependentHandles[dependencyIdx];

                        handleLoadTask.SetDependency(taskDependencies[dependencyIdx], &dependencyTask);
                        dependencyIdx++;
                    }

                    for (ResolveAssetDependencyTask& task : referenceTasks)
                    {
                        scheduler->AddTaskSetToPipe(&task);
                    }
                }
                else
                {
                    scheduler->AddTaskSetToPipe(&handleLoadTask);
                }

                scheduler->WaitforTask(&handleLoadTask);
                while (handleLoadTask.doReschedule)
                {
                    // Reschedule the task if not all of its dependencies were ready at time of load
                    handleLoadTask.doReschedule = false;
                    scheduler->AddTaskSetToPipe(&handleLoadTask);
                    scheduler->WaitforTask(&handleLoadTask);
                }
            }
            else
            {
                ScopedVector<Asset> dependencies;
                dependencies.reserve(asset.references.size());
                for (std::string_view reference : asset.references)
                {
                    dependencies.push_back(LoadInternal(reference, flags));
                }
                

                bank->Store(handle.second, {
                    .identifier = identifier,
                    .data = asset.assetData,
                    .dependencies = dependencies,
                    .registry = this
                });
            }
        }

        return handle.second;
    }
}