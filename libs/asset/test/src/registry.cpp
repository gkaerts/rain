#include <gtest/gtest.h>
#include "asset/registry.hpp"

#include "asset_gen.hpp"
#include "luagen/schema.hpp"

using namespace rn;

struct TestType
{
    uint32_t data;
};

class TestAssetBuilder : public asset::Builder<TestType>
{
public:

    TestType Build(const asset::AssetBuildDesc& desc) override
    {
        RN_ASSERT(desc.data.size() == sizeof(TestType));

        TestType assetData;
        std::memcpy(&assetData, desc.data.data(), sizeof(TestType));
        
        return assetData;
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
    registry.RegisterAssetType<TestType>({
        .extensionHash = HashString(".test_asset"),
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
    registry.RegisterAssetType<TestType>({
        .extensionHash = HashString(".test_asset"),
        .initialCapacity = 16,
        .builder = &testAssetBuilder
    });

    asset::AssetIdentifier handle1 = asset::MakeAssetIdentifier("test_asset_1.test_asset");
    registry.Load("test_asset_1.test_asset");

    const TestType* data1 = registry.Resolve<TestType>(handle1);
    EXPECT_EQ(data1->data, 0xDEADBEEF);

    asset::AssetIdentifier handle2 = asset::MakeAssetIdentifier("test_asset_2.test_asset");
    registry.Load("test_asset_2.test_asset");

    const TestType* data2 = registry.Resolve<TestType>(handle2);
    EXPECT_EQ(data2->data, 0xDABABADA);
}