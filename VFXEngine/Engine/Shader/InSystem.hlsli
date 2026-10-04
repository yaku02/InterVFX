struct DescIndices
{
    uint globalIndicesCBVIndex;
    uint passIndicesCBVIndex;
    uint assetIndicesCBVIndex;
    uint instIndicesCBVIndex;
};

ConstantBuffer<DescIndices> g_descIndices : register(b0);

struct GlobalDescIndices
{
    uint paramCBVIndex;
};

struct GlobalParam
{
    float mainDeltaTime;
    float3 padding;
};

ConstantBuffer<GlobalDescIndices> GetGlobalIndices()
{
    return ResourceDescriptorHeap[g_descIndices.globalIndicesCBVIndex];
}

ConstantBuffer<GlobalParam> GetGlobalParam()
{
    uint cbvIndex = GetGlobalIndices().paramCBVIndex;
    return ResourceDescriptorHeap[cbvIndex];
}