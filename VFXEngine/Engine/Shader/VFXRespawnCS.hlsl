#include "VFXSystem.hlsli"

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
    // ★ 今フレームで発生させる数（emitCount）を超えたスレッドは終了
    if (idx >= assetParam.emitCount)
        return;

    GlobalParam globalParam = GetVFXGlobalParam();
    VFXInstParam instParam = GetVFXInstParam();
    VFXPassParam passParam = GetVFXPassParam();
    
    RWStructuredBuffer<Particle> particles = GetParticleRW();
    
    // ★ Consume バッファとして取得
    ConsumeStructuredBuffer<uint> deadList = GetDeadListConsume();

    // ★ デッドリストから空きインデックスを自動で1つ取得（ポップ）
    // ※ ハードウェアのカウンターによって自動制御されるため、枯渇時の安全装置も基本的には自動です
    uint particleIndex = deadList.Consume();
    if (particleIndex >= assetParam.numParticle)
    {
        return;
    }
    // --- 新規パーティクルの初期化 ---
    Particle p;
    p.age = 0;
    p.position = float3(0, 0, 0);

    p.seed = particleIndex * 1973u + uint(globalParam.globalDeltaTime * 100000.0f) + uint(passParam.vfxPassNoise * 10.0f);
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
    p.color = assetParam.startColor;
    p.size = 1.0f;
    p.custom = float3(0.0f, 0.0f, 0.0f);

    // デッドリストから取得した実際のパーティクルインデックスに初期値を書き込む
    particles[particleIndex] = p;
    // ★ アクティブリストに追加する
    // AppendStructuredBuffer を使うことで、インデックスの登録と裏側のカウンターのインクリメントが同時に行われます
    AppendStructuredBuffer<uint> activeList = GetActiveListAppend();
    activeList.Append(particleIndex);
}