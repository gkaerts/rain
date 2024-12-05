#include <gtest/gtest.h>

#include "common/memory/memory.hpp"
#include "asset_gen.hpp"
#include "reference_gen.hpp"
#include "luagen/schema.hpp"

TEST(SerializationTests, SerializeAndDeserializeAsset)
{
    using namespace rn::asset;

    std::string_view references[] = {
        "reference_one.texture",
        "reference_two.texture"
    };

    uint8_t data[] = { 0xFF, 0xAB, 0xBA, 0xDD };

    schema::Asset preSerialization = {
        .identifier = "test",
        .references = references,
        .assetData = data
    };

    uint64_t destSize = schema::Asset::SerializedSize(preSerialization);
    rn::Span<uint8_t> destSpan = { static_cast<uint8_t*>(rn::ScopedAlloc(destSize, 64)), destSize };

    uint64_t serializedSize = rn::Serialize<schema::Asset>(destSpan, preSerialization);
    EXPECT_EQ(destSize, serializedSize);

    schema::Asset postSerializaiton = rn::Deserialize<schema::Asset>(destSpan, [](size_t size) { return rn::ScopedAlloc(size, 64); });
    EXPECT_EQ(preSerialization.identifier, postSerializaiton.identifier);
    EXPECT_EQ(preSerialization.references.size(), postSerializaiton.references.size());
    EXPECT_EQ(preSerialization.assetData.size(), postSerializaiton.assetData.size());

    for (int i = 0; i < preSerialization.references.size(); ++i)
    {
        EXPECT_EQ(preSerialization.references[i], postSerializaiton.references[i]);
    }

    EXPECT_TRUE(std::memcmp(preSerialization.assetData.data(), postSerializaiton.assetData.data(), preSerialization.assetData.size()) == 0);
}

TEST(SerializationTests, SerializeAndDeserializeReference)
{
    using namespace rn::asset;

    schema::Reference preSerialization = {
        .identifier = 0xABFFDEDE
    };

    uint64_t destSize = schema::Reference::SerializedSize(preSerialization);
    rn::Span<uint8_t> destSpan = { static_cast<uint8_t*>(rn::ScopedAlloc(destSize, 64)), destSize };

    uint64_t offset = 0;
    uint64_t serializedSize = rn::Serialize<schema::Reference>(destSpan, preSerialization);
    EXPECT_EQ(destSize, serializedSize);

    offset = 0;
    schema::Reference postSerializaiton = schema::Reference::Deserialize(destSpan, offset, [](size_t size) { return rn::ScopedAlloc(size, 64); });
    EXPECT_EQ(preSerialization.identifier, postSerializaiton.identifier);
}