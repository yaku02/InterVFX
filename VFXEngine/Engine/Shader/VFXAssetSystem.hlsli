#include "InSystem.hlsli"


struct AssetDescIndices
{
    uint paramCBVIndex;
};

struct AssetParam
{
        // --- Size ---
    float startSize;
    float endSize;
        // --- Behavior ---
    float gravity;
    float drag;
        // --- Color ---
    float4 startColor;
    float4 endColor;
    
    float emissive;
    float lifeTime;
        // --- Noise ---
    float noiseStrength;
    uint numParticle;

};

ConstantBuffer<AssetDescIndices> GetAssetIndices()
{
    return ResourceDescriptorHeap[g_descIndices.assetIndicesCBVIndex];
}

ConstantBuffer<AssetParam> GetAssetParam()
{
    uint cbvIndex = GetAssetIndices().paramCBVIndex;
    return ResourceDescriptorHeap[cbvIndex];
}