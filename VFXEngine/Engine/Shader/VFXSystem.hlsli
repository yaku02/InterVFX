#pragma once

#include "VFXPassSystem.hlsli"
#include "VFXAssetSystem.hlsli"
#include "VFXInstSystem.hlsli"

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

// 各種パラメータやバッファをまとめるコンテキスト構造体
struct EmitContext
{
    uint idx;
    uint particleIndex;
    VFXAssetParam assetParam;
    GlobalParam globalParam;
    VFXInstParam instParam;
    VFXPassParam passParam;
    RWStructuredBuffer<Particle> particles;
    ConsumeStructuredBuffer<uint> deadList;
    AppendStructuredBuffer<uint> activeList;
};

// ① 前処理（Init）：スレッド判定、デッドリストのポップ、基本データの初期化
bool BeginEmit(uint3 id, out EmitContext ctx, out Particle p, ConsumeStructuredBuffer<uint> deadList)
{
        // デッドリストの残り数（空き数）が 0 なら、これ以上スポーンさせない
    RWStructuredBuffer<uint> deadCounter = GetDeadCounterRW();
    if (deadCounter[0] == 0)
    {
        return false;
    }
    
    ctx.idx = id.x;
    ctx.assetParam = GetVFXAssetParam();
    
    // 発生数を超えたスレッドは終了
    if (ctx.idx >= ctx.assetParam.emitCount)
        return false;
    
    ctx.globalParam = GetVFXGlobalParam();
    ctx.instParam = GetVFXInstParam();
    ctx.passParam = GetVFXPassParam();
    
    // デッドリストから空きインデックスを取得
    ctx.particleIndex = deadList.Consume();
    if (ctx.particleIndex >= ctx.assetParam.numParticle)
    {
        return false;
    }

    // パーティクルの基本初期化
    p.age = 0;
    p.position = float3(0, 0, 0);
    p.color = ctx.assetParam.startColor;
    p.size = 1.0f;
    p.custom = float3(0.0f, 0.0f, 0.0f);

    return true;
}

// ③ 後処理（Execute）：バッファへの書き込みとアクティブリストへの登録
void EndEmit(EmitContext ctx, Particle p, RWStructuredBuffer<Particle> particles, AppendStructuredBuffer<uint> activeList)
{
    particles[ctx.particleIndex] = p;
    activeList.Append(ctx.particleIndex);
}