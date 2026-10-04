#include "pch.h"
#include "GraphicsPipeline.h"
#include "Debugger.h"
#include "ShaderCompiler.h"
#include "StringConverter.h"

GraphicsPipeline GraphicsPipeline::Create(ID3D12Device* dev, const Desc& desc)
{
    GraphicsPipeline result;
    ShaderCompiler compiler;

    // 1. ベースとして呼び出し元の d3dDesc をコピー（デフォルト値や個別カスタム設定を継承）
    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = desc.d3dDesc;

    // 2. ルートシグネチャのセット
    psoDesc.pRootSignature = desc.rootSignature.Get();

    // 3. シェーダーコンパイル & バイトコードセット
    auto vsBlob = compiler.CompileShader(desc.vsPath, desc.vsEntry, desc.vsModel);
    auto psBlob = compiler.CompileShader(desc.psPath, desc.psEntry, desc.psModel);

    if (vsBlob) {
        psoDesc.VS = { vsBlob->GetBufferPointer(), vsBlob->GetBufferSize() };
    }
    if (psBlob) {
        psoDesc.PS = { psBlob->GetBufferPointer(), psBlob->GetBufferSize() };
    }

    // 4. ブレンドステートの自動構築（blendMode に応じて設定）
    psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
    D3D12_RENDER_TARGET_BLEND_DESC& rtBlend = psoDesc.BlendState.RenderTarget[0];
    rtBlend.BlendEnable = TRUE;
    rtBlend.SrcBlend = D3D12_BLEND_SRC_ALPHA;
    rtBlend.BlendOp = D3D12_BLEND_OP_ADD;
    rtBlend.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

    if (desc.blendMode == BlendMode::Additive) {
        // 加算合成
        rtBlend.DestBlend = D3D12_BLEND_ONE;
        rtBlend.SrcBlendAlpha = D3D12_BLEND_ONE;
        rtBlend.DestBlendAlpha = D3D12_BLEND_ONE;
        rtBlend.BlendOpAlpha = D3D12_BLEND_OP_ADD;
    }
    else {
        // アルファ半透明
        rtBlend.DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
        rtBlend.SrcBlendAlpha = D3D12_BLEND_ONE;
        rtBlend.DestBlendAlpha = D3D12_BLEND_ZERO;
        rtBlend.BlendOpAlpha = D3D12_BLEND_OP_ADD;
    }

    // 5. DSVFormatがNONE/UNKNOWNの場合は自動でDepthEnableをOFFにする補正
    if (psoDesc.DSVFormat == DXGI_FORMAT_UNKNOWN) {
        psoDesc.DepthStencilState.DepthEnable = FALSE;
    }

    // 6. PSOの生成
    result.rootSignature = desc.rootSignature;
    HRESULT hr = dev->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&result.pipelineState));

    if (FAILED(hr)) {
        Debugger::Log("[Error] Failed to create Graphics PSO: %s / %s\n", desc.vsPath.c_str(), desc.psPath.c_str());
        return result;
    }

    // デバッグ用の名前設定
    std::string psoName = desc.vsEntry + "/" + desc.psEntry;
    result.pipelineState->SetName(StringConverter::ToLPCWSTR(psoName).c_str());

    return result;
}