#pragma once

struct BaseAssetParam
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
    
    float noiseStrength;
    uint numParticles;
};

struct BaseInstParam
{
    float4x4 transform;
    float3 direction;
    float speed;

    uint srvIndex;
    uint uavIndex;
    float2 padding;
};

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