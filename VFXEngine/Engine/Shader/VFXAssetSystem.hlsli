#include "InSystem.hlsli"

struct VFXAssetParam
{
    float startSize;
    float endSize;
    float gravity;
    float drag;
    float4 startColor;
    float4 endColor;
    float emissive;
    float lifeTime;
    float noiseStrength;
    uint numParticle;
};

VFXAssetParam GetVFXAssetParam()
{
    // ConstantBuffer<VFXAssetParam> としてヒープから読み出す
    ConstantBuffer<VFXAssetParam> vfxAssetParamCB = ResourceDescriptorHeap[g_descIndices.globalIndicesCBVIndex];
    return vfxAssetParamCB;
}