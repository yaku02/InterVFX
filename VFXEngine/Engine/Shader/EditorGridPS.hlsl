#include "InSystem.hlsli"

struct VSOutput
{
    float4 Pos : SV_POSITION;
    float2 ndcUV : TEXCOORD0; // NDC座標 (-1.0 ～ 1.0)
};

// -------------------------------------------------------------
// 安定したアンチエイリアスグリッド関数
// -------------------------------------------------------------
float DrawGrid(float2 st, float gridSpacing)
{
    float2 coord = st / gridSpacing;
    // fwidth の極端な肥大化による消滅を防止するガード
    float2 fw = max(fwidth(coord), 0.00001);
    
    // 遠すぎてモアレ（チラつき）が激しくなる領域は線を描画しない
    if (fw.x > 0.4 || fw.y > 0.4)
        return 0.0;

    float2 grid = abs(frac(coord - 0.5) - 0.5) / fw;
    float lineVal = min(grid.x, grid.y);
    return 1.0 - saturate(lineVal);
}

float4 PSMain(VSOutput input) : SV_TARGET
{
    GlobalParam globalParam = GetVFXGlobalParam();

    // 1. mul(Vector, Matrix) の順序で視線レイを復元
    float4 nearWorld = mul(float4(input.ndcUV.x, input.ndcUV.y, 0.0, 1.0), globalParam.invViewProj);
    nearWorld.xyz /= nearWorld.w;

    float4 farWorld = mul(float4(input.ndcUV.x, input.ndcUV.y, 1.0, 1.0), globalParam.invViewProj);
    farWorld.xyz /= farWorld.w;

    float3 rayOrigin = globalParam.cameraPos;
    float3 rayDir = normalize(farWorld.xyz - nearWorld.xyz);

    // 背景色（空）と基本の床色（ダークグレー）
    float3 backgroundColor = float3(0.15, 0.15, 0.15);
    float3 floorBaseColor = float3(0.22, 0.22, 0.22);

    float floorY = -2.0; // 床のY座標

    // 2. Y = floorY 平面との交点判定
    float relativeCamY = rayOrigin.y - floorY;

    if (relativeCamY * rayDir.y < 0.0)
    {
        float t = -relativeCamY / rayDir.y;

        if (t > 0.0)
        {
            // 床の交点座標
            float3 hitPos = rayOrigin + rayDir * t;
            float2 gridPos = hitPos.xz;

            // -------------------------------------------------------------
            // 3. 1m 間隔 (メイン) と 10m 間隔 (サブ) の二段グリッド
            // -------------------------------------------------------------
            float mainGrid = DrawGrid(gridPos, 1.0) * 0.35; // 1mグリッド (薄い)
            float subGrid = DrawGrid(gridPos, 10.0) * 0.70; // 10mグリッド (濃い)
            float gridPattern = max(mainGrid, subGrid);

            // -------------------------------------------------------------
            // 4. X軸 (赤) / Z軸 (青) の中心軸ハイライト
            // -------------------------------------------------------------
            float2 axisWidth = max(fwidth(gridPos), 0.001) * 1.2;
            float axisX = smoothstep(axisWidth.y, 0.0, abs(hitPos.z)) * 0.85; // X軸 (Z=0)
            float axisZ = smoothstep(axisWidth.x, 0.0, abs(hitPos.x)) * 0.85; // Z軸 (X=0)

            // 床色の上にグリッドと軸線を合成
            float3 gridColor = float3(0.5, 0.5, 0.5) * gridPattern;
            gridColor = lerp(gridColor, float3(0.85, 0.25, 0.25), axisX); // 赤 (X軸)
            gridColor = lerp(gridColor, float3(0.25, 0.45, 0.85), axisZ); // 青 (Z軸)

            // -------------------------------------------------------------
            // 5. 距離フェード (遠方のチラつき防止)
            // -------------------------------------------------------------
            float dist = length(hitPos - rayOrigin);
            float distFade = exp(-dist * 0.005); // 遠くに行くにつれて床色に馴染む

            float totalAlpha = saturate(gridPattern + axisX + axisZ) * distFade;
            float3 finalFloorColor = lerp(floorBaseColor, gridColor, totalAlpha);

            return float4(finalFloorColor, 1.0);
        }
    }

    return float4(backgroundColor, 1.0);
}