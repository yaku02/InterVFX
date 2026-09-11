#include "VFXGraphicsSystem.hlsli"

float Hash(uint n)
{
    return frac(sin(n * 12.9898) * 43758.5453);
}

struct VS_OUTPUT
{
    float4 pos : SV_POSITION;
    float4 color : COLOR0;
    float2 uv : TEXCOORD0;
    float3 worldPos : TEXCOORD1;
};

VS_OUTPUT VFXVS(uint vID : SV_VertexID, uint iID : SV_InstanceID)
{
    VS_OUTPUT output;
    Particle p = g_Particles[iID];
    
    float2 offsets[4] =
    {
        float2(-1, -1),
        float2(1, -1),
        float2(-1, 1),
        float2(1, 1)
    };
    
    float2 offset = offsets[vID];

    float3 billboardRight = { g_SceneRes.view[0][0], g_SceneRes.view[1][0], g_SceneRes.view[2][0] };
    float3 billboardUp = { g_SceneRes.view[0][1], g_SceneRes.view[1][1], g_SceneRes.view[2][1] };
    
    float3 center = p.position;

    float currentSize = lerp(g_BaseAssetParam.startSize, g_BaseAssetParam.endSize, saturate(p.age / g_BaseAssetParam.lifeTime));
    
    float3 finalPos = center +
                      offset.x * 0.5f * currentSize * billboardRight +
                      offset.y * 0.5f * currentSize * billboardUp;

    float4 worldPos =
    mul(float4(finalPos, 1), g_BaseInstParam.transform);
    
    output.pos = mul(worldPos, g_SceneRes.viewProj);
    
    output.color = p.color;

    output.uv = offset * 0.5f + 0.5f;
    return output;
}