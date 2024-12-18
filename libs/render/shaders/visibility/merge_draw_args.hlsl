#include "shared/common.h"
#include "shared/visibility/visibility.h"
#include "globals.hlsli"

PushConstants(MergeDrawArgsData, _MergeDrawArgsData)

[numthreads(64, 1, 1)]
void main(uint DTid : SV_DispatchThreadID)
{
    uint drawIdx = DTid;
    if (drawIdx < _MergeDrawArgsData.drawCount)
    {
        // Load instance counts for draw in both source buffers and write their sum to dest buffer
        ByteAddressBuffer srcDrawsA = ResourceDescriptorHeap[_MergeDrawArgsData.buffer_SrcDrawsA];
        ByteAddressBuffer srcDrawsB = ResourceDescriptorHeap[_MergeDrawArgsData.buffer_SrcDrawsB];
        RWByteAddressBuffer destDraws = ResourceDescriptorHeap[_MergeDrawArgsData.rwbuffer_DestDraws];

        uint32_t offset = (drawIdx * sizeof(IndirectIndexedDrawArgs)) + sizeof(uint);
        uint instanceCount = srcDrawsA.Load<uint>(offset) + srcDrawsB.Load<uint>(offset);
        destDraws.Store<uint>(offset, instanceCount);
    }
}