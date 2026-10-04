struct VSOutput
{
    float4 position : SV_POSITION;
    float2 uv : TEXCOORD0;
    float4 color : COLOR0;
};

float4 PSMain(VSOutput input) : SV_TARGET
{
    // 中心から外側に向けて円形に減衰するアルファ値を計算（簡易的なパーティクル模様）
    float2 center = input.uv - 0.5f;
    float dist = length(center);
    float alpha = saturate(1.0f - dist * 2.0f);

    // 円形マスクをかけたカラーを出力
    return float4(input.color.rgb, input.color.a * alpha);
}