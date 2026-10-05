#include "VFXSystem.hlsli"


// 簡易ハッシュ関数（0.0 〜 1.0 のランダム値を生成）
float Hash1D(uint seed)
{
    seed = (seed ^ 61u) ^ (seed >> 16u);
    seed *= 9u;
    seed = seed ^ (seed >> 4u);
    seed *= 0x27d4eb2du;
    seed = seed ^ (seed >> 15u);
    return float(seed) / 4294967295.0f;
}

float3 Hash3D(uint3 seed)
{
    return float3(
        Hash1D(seed.x),
        Hash1D(seed.y + 1999u),
        Hash1D(seed.z + 9999u)
    );
}

[numthreads(64, 1, 1)]
void CSMain(uint3 id : SV_DispatchThreadID)
{
    uint idx = id.x;

    // --- 1. 各スコープのパラメータとバッファを取得 ---
    GlobalParam globalParam = GetVFXGlobalParam();
    VFXPassParam passParam = GetVFXPassParam();
    VFXAssetParam assetParam = GetVFXAssetParam();
    VFXInstParam instParam = GetVFXInstParam();
    
    RWStructuredBuffer<Particle> particles = GetParticleRW();

    if (idx >= assetParam.numParticle)
        return;

    Particle p = particles[idx];

    // --- 2. 時間を進める ---
    p.age += instParam.speed * globalParam.globalDeltaTime;

    // --- 3. 生存中の移動処理 ---
    if (p.age <= assetParam.lifeTime)
    {
        p.velocity.y -= assetParam.gravity * globalParam.globalDeltaTime;
        p.velocity *= max(0.0f, 1.0f - assetParam.drag * globalParam.globalDeltaTime);
        p.position += p.velocity * instParam.speed * globalParam.globalDeltaTime;
    }
    // --- 4. 寿命終了時のリセット（発生処理） ---
    else
    {
        p.age = 0.0f;
        p.position = float3(0.0f, 0.0f, 0.0f);

        p.seed += idx + 1u;

        float3 randVal = Hash3D(uint3(p.seed, p.seed + 13u, p.seed + 37u));

        float randAngle = randVal.x;
        float randHeight = randVal.y;
        float randSpread = randVal.z;

        float minHeight = 3.0f;
        float maxHeight = 6.0f;
        float coneAngle = 0.5f;

        float speedY = lerp(minHeight, maxHeight, randHeight);
        float angle = randAngle * 3.14159265f * 2.0f;
        float radius = speedY * coneAngle * randSpread;

        float speedX = cos(angle) * radius;
        float speedZ = sin(angle) * radius;

        p.velocity = float3(speedX, speedY, speedZ);
    }

    // --- 5. カラー計算（float4 対応） ---
    float ageRate = saturate(p.age / assetParam.lifeTime);
    float4 currentColor = lerp(assetParam.startColor, assetParam.endColor, ageRate);

    // ノイズ適用（RGBのみ補正し、Alphaはスタート〜エンドの補間値をそのまま維持）
    float colorNoise = Hash1D(idx * 7331u + uint(passParam.vfxPassNoise * 100.0f));
    float brightnessOffset = (colorNoise - 0.5f) * assetParam.noiseStrength;

    currentColor.rgb = saturate(currentColor.rgb + brightnessOffset);
    
    // float4 をそのままセット
    p.color = currentColor;

    // --- 6. 結果を書き戻す ---
    particles[idx] = p;
}