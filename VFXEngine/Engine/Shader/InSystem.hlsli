#pragma once
// InSystem.hlsli

// --- ルート定数などから渡されるインデックスバッファ (register(b0)) ---
struct CBVIndices
{
    uint globalCBVIndex;
    uint passCBVIndex;
    uint assetCBVIndex;
    uint instCBVIndex;
};

ConstantBuffer<CBVIndices> g_cbvIndices : register(b0);

// --- GlobalCB のパラメータ（C++側の GlobalCBDesc::Param とサイズを合わせる） ---
struct GlobalParam
{
    float4x4 viewProj; // 64バイト
    float globalDeltaTime; // 4バイト
    float3 padding; // 12バイト -> 計 80バイト
};

struct GlobalIndices
{
    float4 dummy;
};

struct GlobalCB
{
    GlobalParam param;
    GlobalIndices indices;
};

GlobalCB GetGlobalCB()
{
    ConstantBuffer<GlobalCB> cb = ResourceDescriptorHeap[g_cbvIndices.globalCBVIndex];
    return cb;
}

// --- パラメータのみ必要な場合の取得関数 ---
GlobalParam GetVFXGlobalParam()
{
    GlobalCB cb = GetGlobalCB();
    return cb.param;
}

SamplerState PointSampler : register(s0);