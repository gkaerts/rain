#include "visibility/visibility.hlsli"
#include "globals.hlsli"

#ifndef VISIBILITY_SKIP_FRUSTUM_TEST
    #define VISIBILITY_SKIP_FRUSTUM_TEST 0
#endif

#ifndef VISIBILITY_SKIP_HIZ_TEST
    #define VISIBILITY_SKIP_HIZ_TEST 0
#endif

#ifndef VISIBILITY_LOAD_INSTANCES_INDIRECT
    #define VISIBILITY_LOAD_INSTANCES_INDIRECT 0
#endif

#ifndef VISIBILITY_STORE_CULLED_INSTANCES
    #define VISIBILITY_STORE_CULLED_INSTANCES 0
#endif

static const bool _SkipFrustumTest =        VISIBILITY_SKIP_FRUSTUM_TEST;
static const bool _SkipHiZTest =            VISIBILITY_SKIP_HIZ_TEST;
static const bool _LoadInstancesIndirect =  VISIBILITY_LOAD_INSTANCES_INDIRECT;
static const bool _StoreCulledInstances =   VISIBILITY_STORE_CULLED_INSTANCES;

static ConstantBuffer<VisibilityPassData> _PassData = LoadPassData();

[numthreads(VISIBILITY_PASS_GROUP_SIZE, 1, 1)]
void main(uint DTid : SV_DispatchThreadID)
{
    uint instanceCount  = _PassData.sourceInstanceCount;

    if (_LoadInstancesIndirect)
    {
        ByteAddressBuffer instanceCountBuffer = ResourceDescriptorHeap[_PassData.buffer_InstanceCount];
        instanceCount = instanceCountBuffer.Load(0);
    }

    uint visibilityIdx = DTid;
    if (visibilityIdx >= instanceCount)
    {
        return;
    }

    uint instanceIdx = visibilityIdx;
    if (_LoadInstancesIndirect)
    {
        ByteAddressBuffer sourceInstanceIndices = ResourceDescriptorHeap[_PassData.buffer_SourceInstanceIndices];
        instanceIdx = sourceInstanceIndices.Load(visibilityIdx * sizeof(uint));
    }
    
    ByteAddressBuffer instanceAABBs = ResourceDescriptorHeap[_PassData.buffer_InstanceAABBS];
    InstanceAABB aabb = instanceAABBs.Load<InstanceAABB>(instanceIdx * sizeof(InstanceAABB));

    // Coarse frustum visibility test
    if (_SkipFrustumTest || FrustumTest(_PassData.frustum, _PassData.worldToView, aabb.minCorner, aabb.maxCorner))
    {
        Texture2D<float4> hiZ = ResourceDescriptorHeap[_PassData.tex2D_HiZTexture];
        if (_SkipHiZTest || HiZOcclusionTest(aabb, _PassData.worldToProjection, _PassData.viewportDimensions, hiZ))
        {
            RWByteAddressBuffer destDrawArgs = ResourceDescriptorHeap[_PassData.rwbuffer_DestDrawArgs];
            ByteAddressBuffer instanceToDrawMappings = ResourceDescriptorHeap[_PassData.buffer_InstanceToDrawMappings];
            InstanceToDrawMapping mapping = instanceToDrawMappings.Load<InstanceToDrawMapping>(instanceIdx * sizeof(InstanceToDrawMapping));

            for (uint drawIdx = 0; drawIdx < mapping.drawCount; ++drawIdx)
            {
                // Instance count is the second uint in the IndirectIndexedDrawArgs structure
                uint32_t offset = (mapping.firstDrawIndex + drawIdx) * sizeof(IndirectIndexedDrawArgs) + sizeof(uint);
                destDrawArgs.InterlockedAdd(offset, 1);
            }
        }
        else if (_StoreCulledInstances)
        {
            // Store which instances were culled in case we do a secondary pass
            RWByteAddressBuffer culledInstanceCount = ResourceDescriptorHeap[_PassData.rwbuffer_DestCulledInstanceCount];
            RWByteAddressBuffer culledInstanceIndices = ResourceDescriptorHeap[_PassData.rwbuffer_DestCulledInstanceIndices];

            uint culledInstanceOffset = 0;
            culledInstanceCount.InterlockedAdd(0, 1, culledInstanceOffset);
            culledInstanceIndices.Store(culledInstanceOffset * sizeof(uint), instanceIdx);
        }
    }

}