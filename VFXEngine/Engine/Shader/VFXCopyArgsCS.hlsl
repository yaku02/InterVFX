#include "VFXSystem.hlsli"

[numthreads(64, 1, 1)]
void CSMain(uint3 id : SV_DispatchThreadID)
{
    // 1. カウンターバッファから現在の生存数（アクティブなパーティクル数）を取得
    RWStructuredBuffer<uint> activeCounter = GetActiveCounterRW();
    uint liveCount = activeCounter[0];

    // 2. インディレクト引数バッファを取得
    RWStructuredBuffer<uint4> indirectArgs = GetIndirectArgsRW();

    // 3. DrawArguments の InstanceCount（2番目の要素 .y）に生存数を上書きする
    // ※ 構造体の定義：
    // x: vertexCountPerInstance (4)
    // y: instanceCount          (← ここを現在の生存数に書き換える)
    // z: startVertexLocation    (0)
    // w: startInstanceLocation  (0)
    uint4 args = indirectArgs[0];
    args.y = liveCount;
    indirectArgs[0] = args;
}