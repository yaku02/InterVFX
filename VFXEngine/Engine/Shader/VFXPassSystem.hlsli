#include "InSystem.hlsli"

struct PassDescIndices
{
    uint paramCBVIndex;
};

struct PassParam
{
    float4 test;
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