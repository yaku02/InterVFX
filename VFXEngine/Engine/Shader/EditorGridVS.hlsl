#include "InSystem.hlsli"

struct VSOutput
{
    float4 Pos : SV_POSITION;
    float2 ndcUV : TEXCOORD0; // NDC座標 (-1.0 ～ 1.0)
};

VSOutput VSMain(uint vertexID : SV_VertexID)
{
    VSOutput output;
    // フルスクリーン三角形の生成
    float2 uv = float2((vertexID << 1) & 2, vertexID & 2);
    output.Pos = float4(uv * float2(2.0, -2.0) + float2(-1.0, 1.0), 0.0, 1.0);
    
    // NDC 座標 (-1.0 ～ 1.0) を渡す (Y軸は上が +1.0)
    output.ndcUV = float2(output.Pos.x, output.Pos.y);
    
    return output;
}