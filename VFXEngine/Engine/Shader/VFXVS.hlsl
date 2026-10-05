#include "VFXSystem.hlsli"

struct VSOutput
{
    float4 pos : SV_POSITION;
    float4 color : COLOR0;
    float2 uv : TEXCOORD0;
    float3 worldPos : TEXCOORD1;
};

VSOutput VSMain(uint vertexID : SV_VertexID, uint instanceID : SV_InstanceID)
{
    VSOutput output;

    // パラメータ取得
    GlobalParam globalParam = GetVFXGlobalParam();
    VFXAssetParam assetParam = GetVFXAssetParam();
    VFXInstParam instParam = GetVFXInstParam();
    StructuredBuffer<Particle> particles = GetParticleST();

    // instanceID をそのままパーティクルインデックスとして使用
    uint particleIdx = instanceID;
    uint cornerIdx = vertexID;

    if (particleIdx >= assetParam.numParticle)
    {
        output.pos = float4(0, 0, 0, 0);
        return output;
    }

    Particle p = particles[particleIdx];

    // --- 3. ビルボード用オフセットテーブル（TriangleStrip / 4頂点用） ---
    // 0: 左下 (-1, -1), 1: 右下 (1, -1), 2: 左上 (-1, 1), 3: 右上 (1, 1)
    float2 offsets[4] =
    {
        float2(-1.0f, -1.0f),
        float2(1.0f, -1.0f),
        float2(-1.0f, 1.0f),
        float2(1.0f, 1.0f)
    };
    float2 offset = offsets[cornerIdx];

    // --- 4. ビルボードの右・上方向ベクトルの算出（View行列の転置軸） ---
    float3 billboardRight = float3(globalParam.viewProj[0][0], globalParam.viewProj[1][0], globalParam.viewProj[2][0]);
    float3 billboardUp = float3(globalParam.viewProj[0][1], globalParam.viewProj[1][1], globalParam.viewProj[2][1]);

    // --- 5. サイズ補間とローカル位置の計算 ---
    float ageRate = saturate(p.age / assetParam.lifeTime);
    float currentSize = lerp(assetParam.startSize, assetParam.endSize, ageRate);

    float3 localPos = p.position +
                      offset.x * 0.5f * currentSize * billboardRight +
                      offset.y * 0.5f * currentSize * billboardUp;

    // --- 6. 変換行列の適用 (World -> Screen) ---
    float4 worldPos = mul(float4(localPos, 1.0f), instParam.transform);
    
    output.pos = mul(worldPos, globalParam.viewProj);
    output.color = p.color;
    output.uv = offset * 0.5f + 0.5f; // [-1, 1] -> [0, 1] に変換
    output.worldPos = worldPos.xyz;

    return output;
}