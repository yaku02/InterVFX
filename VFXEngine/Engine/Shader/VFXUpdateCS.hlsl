struct VFXPassIndices
{
    uint globalPassCBVIndex;
    uint assetCBVIndex;
    uint instanceCBVIndex;
};

[numthreads(64, 1, 1)]
void CSMain(uint3 dispatchThreadID : SV_DispatchThreadID)
{
    // 今はパイプライン動作テスト用のため処理なし
    // 将来的に StructuredBuffer<Particle> などの更新処理を記述
}