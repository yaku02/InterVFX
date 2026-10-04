#include "InSystem.hlsli"

struct InstDescIndices
{
    uint particleSRVIndex;
    uint particleUAVIndex;
    uint paramCBVIndex;
};

struct Particle
{
    float3 position;
    float size;

    float3 velocity;
    float age;

    float3 color;

    uint seed;
    float3 custom;
};

struct InstParam
{
    float4x4 transform;
    float3 direction;
    float speed;
};

ConstantBuffer<InstDescIndices> GetInstIndices()
{
    return ResourceDescriptorHeap[g_descIndices.instIndicesCBVIndex];
}

StructuredBuffer<Particle> GetParticleST()
{
    uint srvIndex = GetInstIndices().particleSRVIndex;
    return ResourceDescriptorHeap[srvIndex];
}

RWStructuredBuffer<Particle> GetParticleRW()
{
    uint uavIndex = GetInstIndices().particleUAVIndex;
    return ResourceDescriptorHeap[uavIndex];
}

ConstantBuffer<InstParam> GetInstParam()
{
    uint cbvIndex = GetInstIndices().paramCBVIndex;
    return ResourceDescriptorHeap[cbvIndex];
}