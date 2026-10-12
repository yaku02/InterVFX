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
    VFXAssetParam assetParam = GetVFXAssetParam();

    if (idx >= assetParam.numParticle)
        return;

    GlobalParam globalParam = GetVFXGlobalParam();
    VFXInstParam instParam = GetVFXInstParam();
    
    RWStructuredBuffer<Particle> particles = GetParticleRW();
    AppendStructuredBuffer<uint> deadList = GetDeadListAppend();
    
    Particle p = particles[idx];

    // ★ すでに寿命を超えて死んでいる（デッド状態の）パーティクルは、
    // ここで何もしずにスルーする（古い位置を上書きし続けないようにする）
    if (p.age > assetParam.lifeTime)
    {
        return;
    }

    // --- 時間を進める ---
    float prevAge = p.age;
    p.age += instParam.speed * globalParam.globalDeltaTime;

    // --- 寿命を迎えた瞬間の判定 ---
    if (p.age > assetParam.lifeTime)
    {
        deadList.Append(idx);
        particles[idx] = p;
    }
    else
    {
        // 生存中の移動処理
        p.velocity.y -= assetParam.gravity * globalParam.globalDeltaTime;
        p.velocity *= max(0.0f, 1.0f - assetParam.drag * globalParam.globalDeltaTime);
        p.position += p.velocity * instParam.speed * globalParam.globalDeltaTime;

        // カラー計算
        float ageRate = saturate(p.age / assetParam.lifeTime);
        p.color = lerp(assetParam.startColor, assetParam.endColor, ageRate);
        
            // 結果を書き戻す
        particles[idx] = p;

        AppendStructuredBuffer<uint> activeList = GetActiveListAppend();
        activeList.Append(idx);
    }
}