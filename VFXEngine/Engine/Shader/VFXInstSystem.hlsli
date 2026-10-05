#pragma once
// VFXInstSystem.hlsli

#include "InSystem.hlsli"

struct Particle
{
    float3 position;
    float size;

    float3 velocity;
    float age;

    float4 color;

    uint seed;
    float3 custom;
};

struct VFXInstParam
{
    float4x4 transform;
    float3 direction;
    float speed;
};

struct VFXInstIndices
{
    uint particleSRV;
    uint particleUAV;
    float2 padding;
};

struct VFXInstCB
{
    VFXInstParam param;
    VFXInstIndices indices;
};

// --- インスタンス全体の CBV を取得する関数 ---
VFXInstCB GetVFXInstCB()
{
    ConstantBuffer<VFXInstCB> cb = ResourceDescriptorHeap[g_cbvIndices.instCBVIndex];
    return cb;
}

// --- パラメータのみ必要な場合の取得関数 ---
VFXInstParam GetVFXInstParam()
{
    VFXInstCB cb = GetVFXInstCB();
    return cb.param;
}

// --- 読み取り専用 Particle (SRV / StructuredBuffer) の取得 ---
// ※ 戻り値は Particle 1つではなく「バッファ全体」を返すようにします
StructuredBuffer<Particle> GetParticleST()
{
    VFXInstCB cb = GetVFXInstCB();
    StructuredBuffer<Particle> buffer = ResourceDescriptorHeap[cb.indices.particleSRV];
    return buffer;
}

// --- 読み書き用 Particle (UAV / RWStructuredBuffer) の取得 ---
RWStructuredBuffer<Particle> GetParticleRW()
{
    VFXInstCB cb = GetVFXInstCB();
    RWStructuredBuffer<Particle> buffer = ResourceDescriptorHeap[cb.indices.particleUAV];
    return buffer;
}