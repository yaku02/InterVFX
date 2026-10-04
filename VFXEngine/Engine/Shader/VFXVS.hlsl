struct VSOutput
{
    float4 position : SV_POSITION;
    float2 uv : TEXCOORD0;
    float4 color : COLOR0;
};

VSOutput VSMain(uint vertexID : SV_VertexID)
{
    VSOutput output;

    // 四角形（Triangle Strip）のUV位置を自動生成 (0..1)
    float2 uv = float2((vertexID << 1) & 2, vertexID & 2);
    output.uv = uv;

    // 画面中央に1個のビルボードを表示する仮の位置計算 (-0.5 〜 +0.5)
    float2 pos = uv * 2.0f - 1.0f;
    output.position = float4(pos.x, -pos.y, 0.0f, 1.0f);

    // テスト用の描画カラー (緑色)
    output.color = float4(0.0f, 1.0f, 0.0f, 1.0f);

    return output;
}