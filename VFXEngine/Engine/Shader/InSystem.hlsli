// --- ルート定数などから渡されるインデックスバッファ (register(b0)) ---
struct DescIndices
{
    uint globalIndicesCBVIndex;
    uint passIndicesCBVIndex;
    uint assetIndicesCBVIndex;
    uint instIndicesCBVIndex;
};

ConstantBuffer<DescIndices> g_descIndices : register(b0);

// --- GlobalCB のパラメータ（C++側の GlobalCBDesc::Param とサイズを合わせる） ---
struct GlobalParam
{
    float4x4 viewProj; // 64バイト
    float globalDeltaTime; // 4バイト
    float3 padding; // 12バイト -> 計 80バイト
};

// --- バインドレスヒープから GlobalParam 構造体を取得するヘルパー関数 ---
GlobalParam GetGlobalParam()
{
    // ConstantBuffer<GlobalParam> としてヒープから読み出す
    ConstantBuffer<GlobalParam> globalParamCB = ResourceDescriptorHeap[g_descIndices.globalIndicesCBVIndex];
    return globalParamCB;
}