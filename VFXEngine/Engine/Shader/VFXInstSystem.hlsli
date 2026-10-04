#include "InSystem.hlsli"

struct Particle
{
    float3 position;
    float size;

    float3 velocity;
    float age;

    float3 color;

    uint seed;
    float3 custom;
    float padding; // ★ C++側と16バイト境界を揃えるため追加
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
    ConstantBuffer<VFXInstCB> cb = ResourceDescriptorHeap[g_descIndices.instIndicesCBVIndex];
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
StructuredBuffer<Particle> GetParticleSRV(uint srvIndex)
{
    StructuredBuffer<Particle> buffer = ResourceDescriptorHeap[srvIndex];
    return buffer;
}

// --- 読み書き用 Particle (UAV / RWStructuredBuffer) の取得 ---
RWStructuredBuffer<Particle> GetParticleUAV(uint uavIndex)
{
    RWStructuredBuffer<Particle> buffer = ResourceDescriptorHeap[uavIndex];
    return buffer;
}