#include "VFXSystem.hlsli"

// ② 形状ごとの初期化処理（Cone専用）
void EmitCone(inout Particle p, EmitContext ctx)
{
    // シード値の計算（* 演算子を復元）
    p.seed = ctx.particleIndex * 1973u +
             uint(ctx.globalParam.globalDeltaTime * 100000.0f) +
             uint(ctx.passParam.vfxPassNoise * 10.0f);
    
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

[numthreads(64, 1, 1)]
void CSMain(uint3 id : SV_DispatchThreadID)
{
    EmitContext ctx;
    Particle p;

    // バッファをここで取得
    RWStructuredBuffer<Particle> particles = GetParticleRW();
    ConsumeStructuredBuffer<uint> deadList = GetDeadListConsume();
    AppendStructuredBuffer<uint> activeList = GetActiveListAppend();

    // 1. 初期化 ＆ デッドリスト取得
    if (!BeginEmit(id, ctx, p, deadList))
        return;

    // 2. 形状ごとの初期化
    EmitCone(p, ctx);

    // 3. 書き込み ＆ アクティブリスト登録（バッファを直接渡す）
    EndEmit(ctx, p, particles, activeList);
}