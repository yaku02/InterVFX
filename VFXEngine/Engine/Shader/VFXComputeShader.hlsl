#include "VFXComputeSystem.hlsli"

[numthreads(256, 1, 1)]
void UpdateCS(uint3 id : SV_DispatchThreadID){

    uint idx = id.x;
    if (idx >= g_BaseAssetParam.numParticles)
        return;
    
    ConstantBuffer<BaseAssetParam> assetParam = g_BaseAssetParam;
    ConstantBuffer<BaseInstParam> instParam = g_BaseInstParam;

    Particle p = g_UpdateParticles[idx];
    
    // 1. まず時間を進める
    p.age += instParam.speed * g_SceneRes.deltaTime;
    
    if (p.age <= assetParam.lifeTime)
    {
        p.velocity.y -= assetParam.gravity * g_SceneRes.deltaTime;
        p.velocity *= max(0.0f, 1.0f - assetParam.drag * g_SceneRes.deltaTime);
        p.position += p.velocity * instParam.speed * g_SceneRes.deltaTime;
    }
    // 3. 寿命が来たら、完全に新しく生まれ変わらせる（リセット）
    else
    {
        p.age = 0.0f;
        p.position = float3(0, 0, 0); // 原点からリスタート
        
        // 完全に独立した3つのランダム値を取得 (0.0f 〜 1.0f)
        float3 randVal = Hash3D(uint3(p.seed, p.seed + 13u, p.seed + 37u));
        
        float randAngle = randVal.x; // 円の角度用
        float randHeight = randVal.y; // 上方向の高さ用
        float randSpread = randVal.z; // コーンの「広がり角度」のばらつき用
        
        // --- 逆コーン（円錐）のパラメータ調整項目 ---
        float minHeight = 3.0f; // 最低限の上の強さ
        float maxHeight = 6.0f; // 最大の上の強さ
        float coneAngle = 0.5f; // コーンの開き具合（倍率。大きくすると朝顔型に開く）
        // --------------------------------------------
        
        // 1. まず、上方向への速度（Y）を決める
        float speedY = lerp(minHeight, maxHeight, randHeight);
        
        // 2. 水平方向（XZ平面）の円の角度を計算 (0 〜 2*PI)
        float angle = randAngle * 3.14159265f * 2.0f;
        
        // 3. 【ここが修正ポイント！】
        // 上への勢い（speedY）に比例して、横への広がりを大きくする。
        // これによって、上にいけばいくほど横に広がる「円錐」になります。
        float radius = speedY * coneAngle * randSpread;
        
        // 4. 三角関数を使って、綺麗な真円の方向にベクトルを割り振る
        float speedX = cos(angle) * radius;
        float speedZ = sin(angle) * radius;
        
        // 最終的な速度を適用
        p.velocity = float3(speedX, speedY, speedZ);
    }
    
    float ageRate = saturate(p.age / assetParam.lifeTime);
    float4 currentColor = lerp(assetParam.startColor, assetParam.endColor, ageRate);
// 1. スレッド固有のシードから 0.0 〜 1.0 のランダム値を1つ取り出す
    // ※ Hash1D は前の手順で作ったビット演算ハッシュが安全です
    float colorNoise = Hash1D(idx * 7331u);

    // 2. ノイズを -0.5 〜 +0.5 の範囲に変換して、明暗の補正値にする
    float brightnessOffset = (colorNoise - 0.5f) * assetParam.noiseStrength;

    // 3. RGBにそれぞれ足し算する（Alphaは変えない）
    currentColor.rgb += brightnessOffset;
    p.color = saturate(currentColor);
    
    g_UpdateParticles[idx] = p;
}

[numthreads(256, 1, 1)]
void FroxelInjectCS(uint3 id : SV_DispatchThreadID)
{
    uint idx = id.x;
    
    Particle p = g_InjectParticles[idx];

    float3 worldPos =
    mul(float4(p.position, 1.0f), g_BaseInstParam.transform).xyz;
    
    uint froxelIndex = GetFroxelIndex(worldPos, g_SceneRes.viewProj, g_FroxelSetting.gridX, g_FroxelSetting.gridY, g_FroxelSetting.gridZ);
    if (froxelIndex == 0xFFFFFFFF)
    {
        return;
    } 
    
    // accumulation
    InterlockedAdd(g_OutputFroxels[froxelIndex].particleCount, 1);
    
    // emissive
    uint emissiveR = (uint) (p.color.r * 1024.0f);
    uint emissiveG = (uint) (p.color.g * 1024.0f);
    uint emissiveB = (uint) (p.color.b * 1024.0f);
    
    InterlockedAdd(g_OutputFroxels[froxelIndex].emissiveR, emissiveR);
    InterlockedAdd(g_OutputFroxels[froxelIndex].emissiveG, emissiveG);
    InterlockedAdd(g_OutputFroxels[froxelIndex].emissiveB, emissiveB);
}