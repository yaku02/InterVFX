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
    uint deadListSRV;
    uint deadListUAV;
    
    uint deadCounterUAV;
    uint activeListSRV;
    uint activeListUAV;
    uint activeCounterUAV;
    
    uint indirectArgsUAV;
    float3 padding;
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

ConsumeStructuredBuffer<uint> GetDeadListConsume()
{
    VFXInstCB cb = GetVFXInstCB();
    ConsumeStructuredBuffer<uint> buffer = ResourceDescriptorHeap[cb.indices.deadListUAV];
    return buffer;
}

AppendStructuredBuffer<uint> GetDeadListAppend()
{
    VFXInstCB cb = GetVFXInstCB();
    AppendStructuredBuffer<uint> buffer = ResourceDescriptorHeap[cb.indices.deadListUAV];
    return buffer;
}

RWStructuredBuffer<uint> GetDeadCounterRW()
{
    VFXInstCB cb = GetVFXInstCB();
    RWStructuredBuffer<uint> buffer = ResourceDescriptorHeap[cb.indices.deadCounterUAV];
    return buffer;
}

// --- 読み取り専用 ActiveList (SRV / StructuredBuffer) の取得 ---
StructuredBuffer<uint> GetActiveListST()
{
    VFXInstCB cb = GetVFXInstCB();
    StructuredBuffer<uint> buffer = ResourceDescriptorHeap[cb.indices.activeListSRV];
    return buffer;
}

// --- 追加用 ActiveList (AppendStructuredBuffer) の取得 ---
AppendStructuredBuffer<uint> GetActiveListAppend()
{
    VFXInstCB cb = GetVFXInstCB();
    AppendStructuredBuffer<uint> buffer = ResourceDescriptorHeap[cb.indices.activeListUAV];
    return buffer;
}

RWStructuredBuffer<uint> GetActiveCounterRW()
{
    VFXInstCB cb = GetVFXInstCB();
    RWStructuredBuffer<uint> buffer = ResourceDescriptorHeap[cb.indices.activeCounterUAV];
    return buffer;
}

// --- 読み書き用 / 更新用 IndirectArgs (UAV / RWStructuredBuffer または型付きバッファ) の取得 ---
// ※ DrawArguments 構造体または uint4 配列として扱えるように RWStructuredBuffer<uint4> や専用構造体で受け取ります
RWStructuredBuffer<uint4> GetIndirectArgsRW()
{
    VFXInstCB cb = GetVFXInstCB();
    RWStructuredBuffer<uint4> buffer = ResourceDescriptorHeap[cb.indices.indirectArgsUAV];
    return buffer;
}
