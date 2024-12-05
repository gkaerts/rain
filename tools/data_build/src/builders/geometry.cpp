#pragma warning(push, 1)
#include <pxr/usd/usdGeom/subset.h>
#include <pxr/usd/usdGeom/primvarsAPI.h>
#pragma warning(pop)


#include "geometry.hpp"
#include "data/geometry.hpp"

#include "common/memory/vector.hpp"
#include "common/math/math.hpp"

#include "rhi/resource.hpp"

#include "meshoptimizer.h"
#include "mikktspace.h"

#include "geometry_gen.hpp"
#include "luagen/schema.hpp"

namespace rn
{
    struct RawMeshlet
    {
        uint32_t vertexOffset;
        uint32_t triangleOffset;
        uint32_t vertexCount;
        uint32_t triangleCount;
    };

    struct RawGeometryPart
    {
        uint32_t materialIdx;
        uint32_t baseVertex;
        Vector<uint32_t> indices;
        Vector<uint16_t> indices16;

        Vector<meshopt_Meshlet> meshlets;
        Vector<uint32_t> meshletVertices;
        Vector<uint8_t> meshletIndices;
    };

    struct RawGeometryData
    {
        Vector<pxr::GfVec3f> positions;
        Vector<pxr::GfVec3f> normals;
        Vector<pxr::GfVec2f> texcoords;
        Vector<pxr::GfVec4f> tangents;

        Vector<RawGeometryPart> parts;

        Vector<uint32_t> tempTriIndices;    // Not needed for export, but used to represent a properly triangulated mesh for optimization

        pxr::GfRange3f aabb;
    };

    

    void RemapVertexStreamsAndIndices(RawGeometryData& geo)
    {
        Vector<meshopt_Stream> streams;
        streams.push_back({
            .data = geo.positions.data(),
            .size = sizeof(geo.positions[0]),
            .stride = sizeof(geo.positions[0])
        });

        streams.push_back({
            .data = geo.normals.data(),
            .size = sizeof(geo.normals[0]),
            .stride = sizeof(geo.normals[0])
        });

        if (!geo.texcoords.empty())
        {
            streams.push_back({
                .data = geo.texcoords.data(),
                .size = sizeof(geo.texcoords[0]),
                .stride = sizeof(geo.texcoords[0])
            });
        }

        if (!geo.tangents.empty())
        {
            streams.push_back({
                .data = geo.tangents.data(),
                .size = sizeof(geo.tangents[0]),
                .stride = sizeof(geo.tangents[0])
            });
        }

        Vector<uint32_t> remap;
        remap.resize(geo.positions.size());
        size_t uniqueVertices = meshopt_generateVertexRemapMulti(remap.data(), geo.tempTriIndices.data(), geo.tempTriIndices.size(), geo.positions.size(), streams.data(), streams.size());

        Vector<pxr::GfVec3f> remappedPositions;
        Vector<pxr::GfVec3f> remappedNormals;
        Vector<pxr::GfVec2f> remappedTexcoords;
        Vector<pxr::GfVec4f> remappedTangents;
        remappedPositions.resize(uniqueVertices);
        remappedNormals.resize(uniqueVertices);

        meshopt_remapVertexBuffer(remappedPositions.data(), geo.positions.data(), geo.positions.size(), sizeof(geo.positions[0]), remap.data());
        meshopt_remapVertexBuffer(remappedNormals.data(), geo.normals.data(), geo.normals.size(), sizeof(geo.normals[0]), remap.data());
        if (!geo.texcoords.empty())
        {
            remappedTexcoords.resize(uniqueVertices);
            meshopt_remapVertexBuffer(remappedTexcoords.data(), geo.texcoords.data(), geo.texcoords.size(), sizeof(geo.texcoords[0]), remap.data());
        }

        if (!geo.tangents.empty())
        {
            remappedTangents.resize(uniqueVertices);
            meshopt_remapVertexBuffer(remappedTangents.data(), geo.tangents.data(), geo.tangents.size(), sizeof(geo.tangents[0]), remap.data());
        }

        for (RawGeometryPart& part : geo.parts)
        {
            Vector<uint32_t> remappedIndices;
            remappedIndices.resize(part.indices.size());
            meshopt_remapIndexBuffer(remappedIndices.data(), part.indices.data(), part.indices.size(), remap.data());

            part.indices = std::move(remappedIndices);
        }

        geo.positions = std::move(remappedPositions);
        geo.normals = std::move(remappedNormals);
        geo.texcoords = std::move(remappedTexcoords);
        geo.tangents = std::move(remappedTangents);
    }

    void OptimizeVertexCache(RawGeometryData& geo)
    {
        for (RawGeometryPart& part : geo.parts)
        {
            Vector<uint32_t> optimizedIndices;
            optimizedIndices.reserve(part.indices.size());
            meshopt_optimizeVertexCache(optimizedIndices.data(), part.indices.data(), part.indices.size(), geo.positions.size());
        }
    }

    void Build16BitIndicesIfPossible(RawGeometryData& geo)
    {
        bool allGeoCanFitWithin16BitIndices = geo.positions.size() <= UINT16_MAX;
        for (RawGeometryPart& part : geo.parts)
        {
            bool canUse16BitIndices = allGeoCanFitWithin16BitIndices;
            if (!canUse16BitIndices)
            {
                // Analyze this part's index buffer and see if we can rebase it using a start vertex
                uint32_t minIndex = 0xFFFFFFFF;
                uint32_t maxIndex = 0;

                for (uint32_t idx : part.indices)
                {
                    minIndex = std::min(minIndex, idx);
                    maxIndex = std::max(maxIndex, idx);
                }

                uint32_t diff = maxIndex - minIndex;
                if (diff <= UINT16_MAX)
                {
                    canUse16BitIndices = true;
                    part.baseVertex = minIndex;
                }
            }

            if (canUse16BitIndices)
            {
                part.indices16.clear();
                part.indices16.resize(part.indices.size());
                for (uint32_t idx : part.indices)
                {
                    part.indices16.push_back(uint16_t(idx - part.baseVertex));
                }
            }
        }
    }

    void BuildMeshlets(RawGeometryData& geo, uint32_t maxVerticesPerMeshlet, uint32_t maxTrianglesPerMeshlet)
    {
        for (RawGeometryPart& part : geo.parts)
        {
            size_t maxMeshletCount = meshopt_buildMeshletsBound(part.indices.size(), maxVerticesPerMeshlet, maxTrianglesPerMeshlet);
            part.meshlets.resize(maxMeshletCount);
            part.meshletVertices.resize(maxMeshletCount * maxVerticesPerMeshlet);
            part.meshletIndices.reserve(maxMeshletCount * maxTrianglesPerMeshlet * 3);

            size_t actualMeshletCount = meshopt_buildMeshlets(
                part.meshlets.data(),
                part.meshletVertices.data(),
                part.meshletIndices.data(),
                part.indices.data(),
                part.indices.size(),
                &geo.positions[0][0],
                geo.positions.size(),
                sizeof(geo.positions[0]),
                maxVerticesPerMeshlet,
                maxTrianglesPerMeshlet,
                0.0f);
            
            part.meshlets.resize(actualMeshletCount);

            const meshopt_Meshlet& lastMeshlet = part.meshlets.back();
            part.meshletVertices.resize(lastMeshlet.vertex_offset + lastMeshlet.vertex_count);
            part.meshletIndices.resize(lastMeshlet.triangle_offset + ((lastMeshlet.triangle_count * 3 + 3) & ~3));
        }
    }

    constexpr const uint32_t MAX_VERTICES_PER_MESHLET = 64;
    constexpr const uint32_t MAX_TRIANGLES_PER_MESHLET = 124;
    bool OptimizeAndPackGeometry(RawGeometryData& geo)
    {
        RemapVertexStreamsAndIndices(geo);
        OptimizeVertexCache(geo);
        Build16BitIndicesIfPossible(geo);
        BuildMeshlets(geo, MAX_VERTICES_PER_MESHLET, MAX_TRIANGLES_PER_MESHLET);

        return true;
    }

    // Is this winding right?
    constexpr const int QUAD_TRIANGULATION_INDICES[] = { 0, 1, 2, 0, 2, 3 };

    bool ProcessUsdPoints(std::string_view file, 
        const DataBuildOptions& options, 
        const pxr::UsdGeomMesh& mesh, 
        float unitScale, 
        FnPermuteVec3 fnPermute, 
        Vector<pxr::GfVec3f>& outPositions,
        Vector<uint32_t>& outTempIndices)
    {
        pxr::VtArray<int> faceVertexCounts;
        pxr::VtArray<int> faceVertexIndices;
        mesh.GetFaceVertexCountsAttr().Get(&faceVertexCounts);
        mesh.GetFaceVertexIndicesAttr().Get(&faceVertexIndices);

        size_t faceCount = mesh.GetFaceCount();
        outPositions.reserve(faceCount * 4);
        outTempIndices.reserve(faceCount * 6);

        pxr::VtArray<pxr::GfVec3f> vtPoints;
        mesh.GetPointsAttr().Get(&vtPoints);

        int indexOffset = 0;
        for (size_t faceIdx = 0; faceIdx < faceCount; ++faceIdx)
        {
            int faceVertexCount = faceVertexCounts[faceIdx];
            RN_ASSERT(faceVertexCount == 3 || faceVertexCount == 4);
            for (int i = 0; i < faceVertexCount; ++i)
            {
                int index = faceVertexIndices[indexOffset + i];
                outPositions.emplace_back(fnPermute((unitScale * vtPoints[index])));
            }

            if (faceVertexCount == 4)
            {
                for (int i : QUAD_TRIANGULATION_INDICES)
                {
                    outTempIndices.push_back(indexOffset + i);
                }
            }
            else
            {
                for (int i = 0; i < faceVertexCount; ++i)
                {
                    outTempIndices.push_back(indexOffset + i);
                }
            }

            indexOffset += faceVertexCount;
        }

        return true;
    }

    const pxr::TfToken PRIMVAR_NORMALS = pxr::TfToken("normals");

    const pxr::TfToken PRIMVARS_TEXCOORDS[] = {
        pxr::TfToken("st"),
        pxr::TfToken("uv"),
        pxr::TfToken("UVMap")
    };

    bool ProcessUsdNormals(std::string_view file, 
        const DataBuildOptions& options, 
        const pxr::UsdGeomMesh& mesh, 
        FnPermuteVec3 fnPermute, 
        Vector<pxr::GfVec3f>& outNormals)
    {
        pxr::VtArray<int> faceVertexCounts;
        mesh.GetFaceVertexCountsAttr().Get(&faceVertexCounts);

        size_t faceCount = mesh.GetFaceCount();
        
        pxr::VtArray<pxr::GfVec3f> vtNormals;
        if (!mesh.GetNormalsAttr().Get(&vtNormals))
        {
            // No standard normals attribute, look for primvars
            pxr::UsdGeomPrimvarsAPI primvarAPI(mesh.GetPrim());
            if (!primvarAPI.GetPrimvar(PRIMVAR_NORMALS).Get(&vtNormals))
            {
                BuildError(file) << "USD: No normals found (UsdGeomMesh: \"" << mesh.GetPath() << "\")." << std::endl;
                return false;
            }
        }

        outNormals.reserve(vtNormals.size());
        for (const pxr::GfVec3f& vtNormal : vtNormals)
        {
            outNormals.emplace_back(fnPermute(vtNormal));
        }

        return true;
    }

    bool ProcessUsdTexcoords(std::string_view file, 
        const DataBuildOptions& options, 
        const pxr::UsdGeomMesh& mesh,
        Vector<pxr::GfVec2f>& outTexcoords)
    {
        pxr::VtArray<int> faceVertexCounts;
        mesh.GetFaceVertexCountsAttr().Get(&faceVertexCounts);

        size_t faceCount = mesh.GetFaceCount();

        pxr::VtArray<pxr::GfVec2f> vtTexcoords;
    
        // Look for possible texcoord primvars
        pxr::UsdGeomPrimvarsAPI primvarAPI(mesh.GetPrim());
        for (const pxr::TfToken& token : PRIMVARS_TEXCOORDS)
        {
            if (primvarAPI.GetPrimvar(token).Get(&vtTexcoords))
            {
                break;
            }
        }
        
        if (vtTexcoords.empty())
        {
            // No texcoords found. This is ok!
            return true;
        }

        outTexcoords.reserve(vtTexcoords.size());
        for (const pxr::GfVec2f& vtTexcoord : vtTexcoords)
        {
            outTexcoords.emplace_back(vtTexcoord);
        }

        return true;
    }

    SMikkTSpaceInterface MIKKT_INTERFACE
    {
        .m_getNumFaces = [](const SMikkTSpaceContext* pContext) -> int
        {
            RawGeometryData* geo = static_cast<RawGeometryData*>(pContext->m_pUserData);
            return int(geo->tempTriIndices.size() / 3);
        },

        .m_getNumVerticesOfFace = [](const SMikkTSpaceContext* pContext, const int iFace) -> int
        {
            return 3;
        },

        .m_getPosition = [](const SMikkTSpaceContext* pContext, float fvPosOut[], const int iFace, const int iVert)
        {
            RawGeometryData* geo = static_cast<RawGeometryData*>(pContext->m_pUserData);
            uint32_t vIdx = geo->tempTriIndices[iFace * 3 + iVert];
            std::memcpy(fvPosOut, &geo->positions[vIdx], sizeof(geo->positions[0]));
        },

        .m_getNormal = [](const SMikkTSpaceContext* pContext, float fvNormOut[], const int iFace, const int iVert)
        {
            RawGeometryData* geo = static_cast<RawGeometryData*>(pContext->m_pUserData);
            uint32_t vIdx = geo->tempTriIndices[iFace * 3 + iVert];
            std::memcpy(fvNormOut, &geo->normals[vIdx], sizeof(geo->normals[0]));
        },

        .m_getTexCoord = [](const SMikkTSpaceContext* pContext, float fvTexcOut[], const int iFace, const int iVert)
        {
            RawGeometryData* geo = static_cast<RawGeometryData*>(pContext->m_pUserData);
            uint32_t vIdx = geo->tempTriIndices[iFace * 3 + iVert];
            std::memcpy(fvTexcOut, &geo->texcoords[vIdx], sizeof(geo->texcoords[0]));
        },

        .m_setTSpaceBasic = [](const SMikkTSpaceContext* pContext, const float fvTangent[], const float fSign, const int iFace, const int iVert)
        {
            RawGeometryData* geo = static_cast<RawGeometryData*>(pContext->m_pUserData);
            uint32_t vIdx = geo->tempTriIndices[iFace * 3 + iVert];
            geo->tangents[vIdx] = pxr::GfVec4f(fvTangent[0], fvTangent[1], fvTangent[2], fSign);
        }
    };

    bool BuildTangents(std::string_view file, const DataBuildOptions& options, RawGeometryData& geo)
    {
        if (geo.normals.empty() || geo.texcoords.empty())
        {
            // Don't need tangents
            return true;
        }

        geo.tangents.resize(geo.normals.size());
        const SMikkTSpaceContext mikktContext
		{
			.m_pInterface = &MIKKT_INTERFACE,
			.m_pUserData = &geo
		};

		genTangSpaceDefault(&mikktContext);
        return true;
    }

    const pxr::TfToken MATERIAL_BIND = pxr::TfToken("materialBind");
    bool ProcessUsdGeomSubsets(std::string_view file, const DataBuildOptions& options, const pxr::UsdGeomMesh& mesh, const Vector<size_t>& faceStartIndices, Vector<RawGeometryPart>& outParts)
    {
        std::vector<pxr::UsdGeomSubset> subsets = pxr::UsdGeomSubset::GetAllGeomSubsets(mesh);
        size_t faceCount = mesh.GetFaceCount();

        pxr::VtArray<int> faceVertexCounts;
        pxr::VtArray<int> faceVertexIndices;
        mesh.GetFaceVertexCountsAttr().Get(&faceVertexCounts);
        mesh.GetFaceVertexIndicesAttr().Get(&faceVertexIndices);

        if (subsets.empty())
        {
            // No explicitly defined subsets, we treat this as our target geometry having one part
            // We'll make a dumb flat index buffer which we'll optimize later
            Vector<uint32_t> indices;
            indices.reserve(faceCount * 6);

            int indexOffset = 0;
            for (size_t faceIdx = 0; faceIdx < faceCount; ++faceIdx)
            {
                int vertexCount = faceVertexCounts[faceIdx];
                RN_ASSERT(vertexCount == 3 || vertexCount == 4);

                if (vertexCount == 4)
                {
                    for (int i : QUAD_TRIANGULATION_INDICES)
                    {
                        indices.push_back(indexOffset + i);
                    }
                }
                else
                {
                    for (int i = 0; i < vertexCount; ++i)
                    {
                        indices.push_back(indexOffset + i);
                    }
                }

                indexOffset += vertexCount;
            }
            

            outParts.push_back({
                .materialIdx = 0,
                .indices = std::move(indices)
            });
        }
        else
        {
            uint32_t materialIdx = 0;
            for (const pxr::UsdGeomSubset& subset : subsets)
            {
                pxr::TfToken elementType;
                subset.GetElementTypeAttr().Get(&elementType);
                if (elementType != pxr::UsdGeomTokens->face)
                {
                    BuildError(file) << "USD: Unsupported elementType found for UsdGeomSubset: \"" << elementType << "\" (UsdGeomSubset: \"" << subset.GetPath() << "\")." << std::endl;
                    return false;
                }

                pxr::TfToken familyName;
                subset.GetFamilyNameAttr().Get(&familyName);
                if (familyName != MATERIAL_BIND)
                {
                    BuildError(file) << "USD: Unsupported familyName found for UsdGeomSubset: \"" << familyName << "\" (UsdGeomSubset: \"" << subset.GetPath() << "\")." << std::endl;
                    return false;
                }
                
                pxr::VtArray<int> vtFaceIndices;
                subset.GetIndicesAttr().Get(&vtFaceIndices);

                Vector<uint32_t> indices;
                indices.reserve(faceCount * 4);

                for (int faceIdx : vtFaceIndices)
                {
                    int vertexCount = faceVertexCounts[faceIdx];
                    RN_ASSERT(vertexCount == 3 || vertexCount == 4);
                    size_t startPointIdx = faceStartIndices[faceIdx];

                    if (vertexCount == 4)
                    {
                        for (int i : QUAD_TRIANGULATION_INDICES)
                        {
                            indices.push_back(faceVertexIndices[startPointIdx + i]);
                        }
                    }
                    else
                    {
                        for (int i = 0; i < vertexCount; ++i)
                        {
                            indices.push_back(faceVertexIndices[startPointIdx + i]);
                        }
                    }
                }

                outParts.push_back({
                    .materialIdx = materialIdx++,
                    .indices = std::move(indices)
                });
            }
        }

        return true;
    }

    pxr::GfRange3f ComputeAABB(std::string_view file, const DataBuildOptions& options, const pxr::UsdGeomMesh& mesh, FnPermuteVec3 fnPermute)
    {
        pxr::UsdTimeCode time = pxr::UsdTimeCode::Default();
        pxr::GfBBox3d bound = mesh.ComputeLocalBound(time, pxr::UsdGeomTokens->default_);
        pxr::GfRange3d range = bound.ComputeAlignedBox();

        pxr::GfVec3f min = fnPermute(pxr::GfVec3f(range.GetMin()));
        pxr::GfVec3f max = fnPermute(pxr::GfVec3f(range.GetMax()));

        pxr::GfVec3f newMin = pxr::GfVec3f(
            std::min(min[0], max[0]),
            std::min(min[1], max[1]),
            std::min(min[2], max[2]));

        pxr::GfVec3f newMax = pxr::GfVec3f(
            std::max(min[0], max[0]),
            std::max(min[1], max[1]),
            std::max(min[2], max[2]));

        return pxr::GfRange3f(newMin, newMax);
    }


    constexpr const uint32_t GEOMETRY_DATA_STREAM_ALIGNMENT = 64;
    template <typename T> data::schema::BufferRegion CalculateStreamAlignedOffsetAndSize(const Vector<T>& vec, size_t& outTotalSizeAppend)
    {
        data::schema::BufferRegion outRegion = {};
        size_t size = 0;
        if (!vec.empty())
        {
            size_t unalignedSize = vec.size() * sizeof(vec[0]);
            size_t alignedSize = AlignSize(unalignedSize, GEOMETRY_DATA_STREAM_ALIGNMENT);
        
            size_t offset = outTotalSizeAppend;
            outTotalSizeAppend += alignedSize;

            outRegion = {
                .offsetInBytes = uint32_t(offset),
                .sizeInBytes = uint32_t(unalignedSize)
            };
        }
        return outRegion;
    }

    template <typename T> void WriteStreamToDataBuffer(uint8_t* data, const Vector<T>& vec, const data::schema::BufferRegion region)
    {
        if (!vec.empty())
        {
            std::memcpy(data + region.offsetInBytes, vec.data(), region.sizeInBytes);
        }
    }

    void AppendStreamIfNotEmpty(
        const data::schema::BufferRegion& region, 
        data::schema::VertexStreamType type, 
        data::VertexStreamFormat format, 
        uint32_t componentCount, 
        uint32_t stride, 
        ScopedVector<data::schema::VertexStream>& outStreams)
    {
        if (region.sizeInBytes > 0)
        {
            outStreams.push_back({
                .region = region,
                .type = type,
                .format = format,
                .componentCount = componentCount,
                .stride = stride
            });
        }
    }

    bool BuildAsset(std::string_view file, const DataBuildOptions& options, std::string meshName, const RawGeometryData& geometry, Vector<std::string>& outFiles)
    {
        using namespace data;
        MemoryScope SCOPE;

        size_t dataBufferSize = 0;

        schema::BufferRegion positionRegion   = CalculateStreamAlignedOffsetAndSize(geometry.positions, dataBufferSize);
        schema::BufferRegion normalsRegion    = CalculateStreamAlignedOffsetAndSize(geometry.normals, dataBufferSize);
        schema::BufferRegion texcoordsRegion  = CalculateStreamAlignedOffsetAndSize(geometry.texcoords, dataBufferSize);
        schema::BufferRegion tangentsRegion   = CalculateStreamAlignedOffsetAndSize(geometry.tangents, dataBufferSize);

        struct PartRegions
        {
            schema::BufferRegion indicesRegion;
            schema::BufferRegion meshletVerticesRegion;
            schema::BufferRegion meshletIndicesRegion;
        };

        ScopedVector<PartRegions> partRegions;
        partRegions.reserve(geometry.parts.size());
        for (const RawGeometryPart& part : geometry.parts)
        {
            partRegions.push_back({
                .indicesRegion          = part.indices16.empty() ? 
                    CalculateStreamAlignedOffsetAndSize(part.indices, dataBufferSize) :
                    CalculateStreamAlignedOffsetAndSize(part.indices16, dataBufferSize),
                .meshletVerticesRegion  = CalculateStreamAlignedOffsetAndSize(part.meshletVertices, dataBufferSize),
                .meshletIndicesRegion   = CalculateStreamAlignedOffsetAndSize(part.meshletIndices, dataBufferSize)
            });
        }

        ScopedVector<uint8_t> dataBuffer(dataBufferSize);
        WriteStreamToDataBuffer(dataBuffer.data(), geometry.positions,  positionRegion);
        WriteStreamToDataBuffer(dataBuffer.data(), geometry.normals,    normalsRegion);
        WriteStreamToDataBuffer(dataBuffer.data(), geometry.texcoords,  texcoordsRegion);
        WriteStreamToDataBuffer(dataBuffer.data(), geometry.tangents,   tangentsRegion);

        for (uint32_t partIdx = 0; const RawGeometryPart& part : geometry.parts)
        {
            const PartRegions& regions = partRegions[partIdx++];

            if (!part.indices16.empty())
            {
                WriteStreamToDataBuffer(dataBuffer.data(), part.indices16, regions.indicesRegion);
            }
            else
            {
                WriteStreamToDataBuffer(dataBuffer.data(), part.indices, regions.indicesRegion);
            }
            WriteStreamToDataBuffer(dataBuffer.data(), part.meshletVertices, regions.meshletVerticesRegion);
            WriteStreamToDataBuffer(dataBuffer.data(), part.meshletIndices, regions.meshletIndicesRegion);
        }

        schema::VertexStream positionStream = {
            .region = positionRegion,
            .type = schema::VertexStreamType::Position,
            .format = data::VertexStreamFormat::Float,
            .componentCount = 3,
            .stride = sizeof(geometry.positions[0])
        };
        
        ScopedVector<schema::VertexStream> streams;
        streams.reserve(16);

        AppendStreamIfNotEmpty(normalsRegion, schema::VertexStreamType::Normal, VertexStreamFormat::Float, 3, sizeof(geometry.normals[0]), streams);
        AppendStreamIfNotEmpty(texcoordsRegion, schema::VertexStreamType::UV, VertexStreamFormat::Float, 2, sizeof(geometry.texcoords[0]), streams);
        AppendStreamIfNotEmpty(tangentsRegion, schema::VertexStreamType::Tangent, VertexStreamFormat::Float, 4, sizeof(geometry.tangents[0]), streams);

        ScopedVector<schema::Meshlet> meshlets;
        ScopedVector<schema::GeometryPart> parts;
        parts.reserve(geometry.parts.size());
        for (uint32_t partIdx = 0; const RawGeometryPart& part : geometry.parts)
        {
            for (const meshopt_Meshlet& meshlet : part.meshlets)
            {
                meshlets.push_back({
                    .vertexOffset = meshlet.vertex_offset,
                    .triangleOffset = meshlet.triangle_offset,
                    .vertexCount = meshlet.vertex_count,
                    .triangleCount = meshlet.triangle_count
                });
            }
        }

        size_t meshletOffset = 0;
        for (uint32_t partIdx = 0; const RawGeometryPart& part : geometry.parts)
        {
            const PartRegions& regions = partRegions[partIdx++];
            parts.push_back({
                .materialIdx = part.materialIdx,
                .baseVertex = part.baseVertex,
                .indices = regions.indicesRegion,
                .indexFormat = part.indices16.empty() ?
                    rhi::IndexFormat::Uint32 :
                    rhi::IndexFormat::Uint16,
                .meshlets = { meshlets.data() + meshletOffset, part.meshlets.size() },
                .meshletVertices = regions.meshletVerticesRegion,
                .meshletIndices = regions.meshletIndicesRegion
            });

            meshletOffset += part.meshlets.size();
        }

        schema::AABB aabb = {
            .min = {
                geometry.aabb.GetMin()[0], 
                geometry.aabb.GetMin()[1],
                geometry.aabb.GetMin()[2]
            },

            .max = {
                geometry.aabb.GetMax()[0], 
                geometry.aabb.GetMax()[1],
                geometry.aabb.GetMax()[2]
            },
        };

        schema::Geometry outGeometry = {
            .aabb = aabb,
            .parts = parts,
            .positions = positionStream,
            .vertexStreams = streams,
            .data = dataBuffer
        };

        uint64_t serializedSize = schema::Geometry::SerializedSize(outGeometry);
        Span<uint8_t> outData = { static_cast<uint8_t*>(ScopedAlloc(serializedSize, CACHE_LINE_TARGET_SIZE)), serializedSize };
        rn::Serialize<schema::Geometry>(outData, outGeometry);

        std::string extension = meshName;
        extension += ".geometry";
        return WriteAssetToDisk(file, extension, options, outData, {}, outFiles) == 0;
    }

    bool ProcessUsdGeomMesh(std::string_view file, const DataBuildOptions& options, const UsdGeometryBuildDesc& desc, Vector<std::string>& outFiles)
    {
        pxr::UsdGeomMesh mesh = desc.mesh;

        pxr::TfToken subdivScheme;
        mesh.GetSubdivisionSchemeAttr().Get(&subdivScheme);

        if (subdivScheme != "none")
        {
            BuildError(file) << "USD: Subdivision scheme \"" << subdivScheme << "\" is not supported (UsdGeomMesh: \"" << mesh.GetPath() << "\")." << std::endl;
            return false;
        }
       

        if (mesh.GetFaceVertexIndicesAttr().GetNumTimeSamples() > 1 ||
            mesh.GetFaceVertexCountsAttr().GetNumTimeSamples() > 1)
        {
            BuildError(file) << "USD: USDGeomMesh has more than one time sample! This is not supported yet! (UsdGeomMesh: \"" << mesh.GetPath() << "\")." << std::endl;
            return false;
        }

        pxr::VtArray<int> faceVertexCounts;
        mesh.GetFaceVertexCountsAttr().Get(&faceVertexCounts);

        size_t faceCount = mesh.GetFaceCount();
        Vector<size_t> faceStartIndices;
        faceStartIndices.reserve(faceCount);

        size_t indexOffset = 0;
        for (size_t faceIdx = 0; faceIdx < faceCount; ++faceIdx)
        {
            int faceVertexCount = faceVertexCounts[faceIdx];
            if (faceVertexCount != 3 && faceVertexCount != 4)
            {
                BuildError(file) << "USD: UsdGeomMesh has faces with an unsupported vertex count: " << faceVertexCount << " (UsdGeomMesh: \"" << mesh.GetPath() << "\")." << std::endl;
                return false;
            }

            faceStartIndices.push_back(indexOffset);
            indexOffset += faceVertexCount;
        }

        RawGeometryData outGeometry = {};

        if (!ProcessUsdPoints(file, options, mesh, desc.unitScale, desc.fnPermute, outGeometry.positions, outGeometry.tempTriIndices) ||
            !ProcessUsdNormals(file, options, mesh, desc.fnPermute, outGeometry.normals) ||
            !ProcessUsdTexcoords(file, options, mesh, outGeometry.texcoords) ||
            !BuildTangents(file, options, outGeometry) ||
            !ProcessUsdGeomSubsets(file, options, mesh, faceStartIndices, outGeometry.parts))
        {
            return false;
        }

        outGeometry.aabb = ComputeAABB(file, options, mesh, desc.fnPermute);

        if (!OptimizeAndPackGeometry(outGeometry))
        {
            return false;
        }

        return BuildAsset(file, options, mesh.GetPrim().GetName().GetString(), outGeometry, outFiles);
    }
}