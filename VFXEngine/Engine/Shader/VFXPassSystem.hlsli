#include "InSystem.hlsli"

struct VFXPassCBDesc
{
    struct Param
    {
        float vfxPassNoise;
        float3 padding;
    }param;
    struct Indices
    {
    }indices;
};

ConstantBuffer<PassDescIndices> GetPassIndices()
{
    return ResourceDescriptorHeap[g_descIndices.passIndicesCBVIndex];
}

ConstantBuffer<PassParam> GetPassParam()
{
    uint cbvIndex = GetPassIndices().paramCBVIndex;
    return ResourceDescriptorHeap[cbvIndex];
}