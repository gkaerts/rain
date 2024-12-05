#include <gtest/gtest.h>
#include "asset/registry.hpp"

#include "asset_gen.hpp"
#include "luagen/schema.hpp"

using namespace rn;

RN_DEFINE_HANDLE(TestHandle, 0x90);
struct TestType
{
    uint32_t data;
};

class TestAssetBuilder : public asset::Builder<TestHandle, TestType>
{
public:

    TestType Build(const asset::AssetBuildDesc& desc) override
    {
        RN_ASSERT(desc.data.size() == sizeof(TestType));
        const TestType* assetData = reinterpret_cast<const TestType*>(desc.data.data());
        return *assetData;
    }

    void Destroy(TestType& data) override {}
    void Finalize() override {}
};

TEST(AssetTests, CanRegisterAssetType)
{
    asset::Registry registry({
        .contentPrefix = ".",
        .enableMultithreadedLoad = false
    });

    TestAssetBuilder testAssetBuilder;
    registry.RegisterAssetType<TestHandle, TestType>({
        .identifierHash = HashString(".test_asset"),
        .initialCapacity = 16,
        .builder = &testAssetBuilder
    });
}

namespace
{
    class MappedTestAsset : public asset::MappedAsset
    {
    public:
        MappedTestAsset(const String& path)
        {
            if (path == "test_asset_1.test_asset")
            {
                TestType data = {
                    .data = 0xDEADBEEF
                };
                
                asset::schema::Asset asset = {
                    .identifier = ".test_asset",
                    .references = {},
                    .assetData = { reinterpret_cast<uint8_t*>(&data), sizeof(data) }
                };

                uint64_t size = asset::schema::Asset::SerializedSize(asset);
                _assetData.resize(size);

                rn::Serialize<asset::schema::Asset>(_assetData, asset);
            }

            else if (path == "test_asset_2.test_asset")
            {
                TestType data = {
                    .data = 0xDABABADA
                };

                asset::schema::Asset asset = {
                    .identifier = ".test_asset",
                    .references = {},
                    .assetData = { reinterpret_cast<uint8_t*>(&data), sizeof(data) }
                };

                uint64_t size = asset::schema::Asset::SerializedSize(asset);
                _assetData.resize(size);

                rn::Serialize<asset::schema::Asset>(_assetData, asset);
            }
        }

        Span<const uint8_t> Ptr() const override
        {
            return _assetData;
        }

        Vector<uint8_t> _assetData;
    };

    asset::MappedAsset* MapTestAsset(MemoryScope& scope, const String& path)
    {
        return ScopedNew<MappedTestAsset>(scope, path);
    }
}

TEST(AssetTests, CanLoadAsset)
{
    
    asset::Registry registry({
        .contentPrefix = "",
        .enableMultithreadedLoad = true,
        .onMapAsset = MapTestAsset
    });

    TestAssetBuilder testAssetBuilder;
    registry.RegisterAssetType<TestHandle, TestType>({
        .identifierHash = HashString(".test_asset"),
        .initialCapacity = 16,
        .builder = &testAssetBuilder
    });

    TestHandle handle1 = registry.Load<TestHandle>("test_asset_1.test_asset");
    EXPECT_TRUE(IsValid(handle1));

    const TestType* data1 = registry.Resolve<TestHandle, TestType>(handle1);
    EXPECT_EQ(data1->data, 0xDEADBEEF);

    TestHandle handle2 = registry.Load<TestHandle>("test_asset_2.test_asset");
    EXPECT_TRUE(IsValid(handle2));

    const TestType* data2 = registry.Resolve<TestHandle, TestType>(handle2);
    EXPECT_EQ(data2->data, 0xDABABADA);
}