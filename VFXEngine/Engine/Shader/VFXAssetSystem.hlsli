#pragma once
// VFXAssetSystem.hlsli

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

struct VFXAssetIndices
{
    float4 dummy;
};

struct VFXAssetCB
{
    VFXAssetParam param;
    VFXAssetIndices indices;
};

// --- アセット全体の CBV を取得する関数 ---
VFXAssetCB GetVFXAssetCB()
{
    ConstantBuffer<VFXAssetCB> cb = ResourceDescriptorHeap[g_cbvIndices.assetCBVIndex];
    return cb;
}

// --- パラメータのみ必要な場合の取得関数 ---
VFXAssetParam GetVFXAssetParam()
{
    VFXAssetCB cb = GetVFXAssetCB();
    return cb.param;
}
