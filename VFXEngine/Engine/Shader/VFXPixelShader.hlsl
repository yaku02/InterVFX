struct PS_INPUT
{
    float4 pos : SV_POSITION;
    float4 color : COLOR0;
    float2 uv : TEXCOORD0;
    float3 worldPos : TEXCOORD1;
};



float4 VFXPS(PS_INPUT input) : SV_TARGET
{
    float2 centerUV = input.uv * 2.0f - 1.0f;
    float distSq = dot(centerUV, centerUV);
    float alphaShape = exp(-distSq * 5.0f);
    float finalAlpha = input.color.a * alphaShape;
    if (finalAlpha < 0.2f)
        discard;
    
    return float4(input.color.xyz, finalAlpha);
}
