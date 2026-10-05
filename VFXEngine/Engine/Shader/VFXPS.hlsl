#include "VFXSystem.hlsli"

struct VSOutput
{
    float4 pos : SV_POSITION;
    float4 color : COLOR0;
    float2 uv : TEXCOORD0;
    float3 worldPos : TEXCOORD1;
};

float4 PSMain(VSOutput input) : SV_TARGET
{
    // --- 1. UV座標を [-1, 1] に変換し、中心からの距離の2乗を計算 ---
    float2 centerUV = input.uv * 2.0f - 1.0f;
    float distSq = dot(centerUV, centerUV);

    // --- 2. ガウシアン減衰で滑らかな円形のアルファ形状を作る ---
    // 中心(0.0)で 1.0、外側に向かって急激に衰退するソフトグラデーション
    float alphaShape = exp(-distSq * 5.0f);

    // --- 3. パーティクル自体のアルファ（色変化やフェード）と乗算 ---
    float finalAlpha = input.color.a * alphaShape;

    // --- 4. アルファテスト（透明度の低いピクセルを破棄） ---
    if (finalAlpha < 0.001f) // ※ 0.2f だと円の縁が途中でバッサリ切れるため、0.001f 程度の小さな値に調整しています
    {
        discard;
    }

    // --- 5. 最終カラーの出力 ---
    return float4(input.color.rgb, finalAlpha);
}