#pragma once

// VFXPassSystem.hlsli

#include "InSystem.hlsli"

struct VFXPassParam
{
    float vfxPassNoise;
    float3 padding;
};

struct VFXPassIndices
{
    float4 dummy;
};

struct VFXPassCB
{
    VFXPassParam param;
    VFXPassIndices indices;
};

// --- パス全体の CBV を取得する関数 ---
VFXPassCB GetVFXPassCB()
{
    ConstantBuffer<VFXPassCB> cb = ResourceDescriptorHeap[g_cbvIndices.passCBVIndex];
    return cb;
}

// --- パラメータのみ必要な場合の取得関数 ---
VFXPassParam GetVFXPassParam()
{
    VFXPassCB cb = GetVFXPassCB();
    return cb.param;
}
