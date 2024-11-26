#include "asset/registry.hpp"
#include "common/memory/string.hpp"
#include "common/log/log.hpp"
#include "mio/mio.hpp"

#include "asset/schema/asset_generated.h"

#include "common/task/scheduler.hpp"
#include "TaskScheduler.h"


namespace rn::asset
{
    RN_DEFINE_LOG_CATEGORY(Asset);

    namespace
    {
        const char* PathExtension(const String& path)
        {
            size_t extOffset = path.find_last_of('.');
            if (extOffset == String::npos)
            {
                return nullptr;
            }

            return path.c_str() + extOffset;
        }

        void SanitizePath(String& path)
        {
            // Forward slashes only
            std::replace(path.begin(), path.end(), '\\', '/');

            // All lower case
            std::transform(path.begin(), path.end(), path.begin(), [](char c)
            {
                return std::tolower(c);
            });

            // No trailing slashes
            if (path.ends_with("/"))
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

            const void* Ptr() const override { return _source.data(); }

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
        SanitizePath(_contentPrefix);
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

    Asset Registry::LoadInternal(const char* identifier, LoadFlags flags)
    {
        MemoryScope SCOPE;
        String path = identifier;

        SanitizePath(path);
        const char* ext = PathExtension(path);

        // Invalid identifier
        RN_ASSERT(ext);
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
            fullPath.append("/");
            fullPath.append(path);

            MappedAsset* mapping = _onMapAsset(SCOPE, path);

            // File is not a valid asset file
            RN_ASSERT(schema::AssetBufferHasIdentifier(mapping->Ptr()));
            const schema::Asset* asset = schema::GetAsset(mapping->Ptr());
            const auto* references = asset->references();

            if (_enableMultithreadedLoad)
            {
                enki::TaskScheduler* scheduler = TaskScheduler();

                struct HandleAssetLoadTask : enki::ITaskSet
                {
                    Registry* registry = nullptr;
                    BankBase* bank = nullptr;
                    const schema::Asset* asset = nullptr;
                    Asset destHandle = Asset::Invalid;
                    const char* identifier;
                    String path;

                    Span<Asset> dependencies;
                    bool doReschedule = false;

                    void ExecuteRange(enki::TaskSetPartition range_, uint32_t threadnum_) override 
                    {
                        bool allDependenciesResident = true;
                        uint32_t dependencyIdx = 0;
                        for (Asset dependency : dependencies)
                        {
                            String ext = PathExtension(asset->references()->Get(dependencyIdx)->c_str());
                            StringHash extHash = HashString(ext);

                            auto bankIt = registry->_extensionHashToBank.find(extHash);
                            RN_ASSERT(bankIt != registry->_extensionHashToBank.end());

                            BankBase* dependencyBank = bankIt->second;
                            if (dependencyBank->AssetResidency(dependency) != Residency::Resident)
                            {
                                LogInfo(LogCategory::Asset, "Asset \"{}\" is waiting on dependency {} ({})", 
                                    path.c_str(), 
                                    dependencyIdx, 
                                    asset->references()->Get(dependencyIdx)->c_str());
                                allDependenciesResident = false;
                                break;
                            }
                            ++dependencyIdx;
                        }

                        if (allDependenciesResident)
                        {
                            bank->Store(destHandle, {
                                .identifier = identifier,
                                .data = { asset->asset_data()->data(), asset->asset_data()->size() },
                                .dependencies = dependencies
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
                        *destHandle = registry->LoadInternal(referencePath.c_str(), flags);
                    }
                };

                size_t referenceCount = references ? references->size() : 0;
                ScopedVector<enki::Dependency> taskDependencies(referenceCount);
                ScopedVector<ResolveAssetDependencyTask> referenceTasks(referenceCount);
                ScopedVector<Asset> dependentHandles(referenceCount);
                ScopedVector<String> sanitizedReferenceStrings(referenceCount);

                HandleAssetLoadTask handleLoadTask;
                handleLoadTask.registry = this;
                handleLoadTask.bank = bank;
                handleLoadTask.asset = asset;
                handleLoadTask.destHandle = handle.second;
                handleLoadTask.identifier = identifier;
                handleLoadTask.path = path;
                handleLoadTask.dependencies = dependentHandles;

                if (references)
                {
                    size_t dependencyIdx = 0;
                    for (const flatbuffers::String* reference : *references)
                    {
                        String& referencePath = sanitizedReferenceStrings[dependencyIdx];
                        referencePath = _contentPrefix;
                        referencePath.append("/");
                        referencePath.append(reference->c_str());
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
                if (references)
                {
                    dependencies.reserve(references->size());
                    for (const flatbuffers::String* reference : *references)
                    {
                        dependencies.push_back(LoadInternal(reference->c_str(), flags));
                    }
                }

                bank->Store(handle.second, {
                    .identifier = identifier,
                    .data = { asset->asset_data()->data(), asset->asset_data()->size() },
                    .dependencies = dependencies
                });
            }
        }

        return handle.second;
    }
}