#include "pch.h"
#include "EditorGridRenderer.h"
#include "GpuResourceManager.h"
#include "RootSignatureManager.h"
#include "RootParamLayout.h"

void EditorGridRenderer::Init(const InitDesc& desc)
{
    m_dev = &desc.device;
    m_gpuResMgr = &desc.gpuResMgr;
    CreatePipeline(m_dev.Get());
}

void EditorGridRenderer::CreatePipeline(ID3D12Device* dev)
{
    const auto& rootSig = RootSignatureManager::GetRootSignature();

    // --- EditorGrid 用共通パラメータの設定 ---
    GraphicsPipeline::Desc baseDesc;
    baseDesc.vsPath = "Engine/Shader/EditorGridVS.hlsl";
    baseDesc.psPath = "Engine/Shader/EditorGridPS.hlsl";
    baseDesc.vsEntry = "VSMain";
    baseDesc.psEntry = "PSMain";
    baseDesc.vsModel = "vs_6_6";
    baseDesc.psModel = "ps_6_6";
    baseDesc.rootSignature = rootSig;

    // 頂点バッファを使用せず SV_VertexID で描画するため、InputLayout は空にする
    baseDesc.d3dDesc.InputLayout = { nullptr, 0 };

    // レンダーターゲット・深度フォーマット設定
    baseDesc.d3dDesc.RTVFormats[0] = DXGI_FORMAT_R16G16B16A16_FLOAT;
    baseDesc.d3dDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
    baseDesc.d3dDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

    // 深度ステート設定: 深度テスト有効・書き込みOFF（背景・最遠点描画のため LESS_EQUAL を指定）
    baseDesc.d3dDesc.DepthStencilState.DepthEnable = TRUE;
    baseDesc.d3dDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
    baseDesc.d3dDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;

    // ラスタライザーステート: カリングなし
    baseDesc.d3dDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;

    // --------------------------------------------------
    // 半透明 (Alpha Blend) PSO の作成
    // --------------------------------------------------
    GraphicsPipeline::Desc gridDesc = baseDesc;
    gridDesc.blendMode = GraphicsPipeline::BlendMode::Alpha; // グリッドの輪郭やグリッド線のアルファ合成用

    m_pipeline = GraphicsPipeline::Create(dev, gridDesc);
}

void EditorGridRenderer::Render(const RenderDesc& desc)
{
    // 1. パイプラインステートとルートシグネチャのセット
    m_pipeline.SetPipeline(desc.cmdList);

    // 2. カメラ行列（GlobalCBV）のインデックスを Root Constants へバインド
    // ※ RootParam やルートシグネチャの構成に合わせてスロット番号等を調整してください
    desc.cmdList->SetGraphicsRoot32BitConstants(
        static_cast<UINT>(RootParam::Slot::CBVIndices),
        1,
        &desc.globalCBVIndex,
        static_cast<UINT>(RootParam::CBVIndices::GlobalCBVIndex)
    );

    // 3. プリミティブトポロジを TRIANGLELIST に設定
    // (全画面三角形を描画するため TRIANGLELIST を使用)
    desc.cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    // 4. 頂点バッファなしで 3 頂点描画（SV_VertexID でシェーダー側で画面全体をカバーする三角形を生成）
    desc.cmdList->DrawInstanced(3, 1, 0, 0);
}