#include "pch.h"
#include "ComputePipeline.h"
#include "ShaderCompiler.h"
#include "Debugger.h"
#include "StringConverter.h"

ComputePipeline ComputePipeline::Create(ID3D12Device* dev, const Desc& desc)
{
    ComputePipeline result;
    ShaderCompiler compiler;

    // 1. ベースとして d3dDesc をコピー（デフォルト値や個別カスタム設定を継承）
    D3D12_COMPUTE_PIPELINE_STATE_DESC psoDesc = desc.d3dDesc;

    // 2. ルートシグネチャのセット
    psoDesc.pRootSignature = desc.rootSignature.Get();

    // 3. CSシェーダーコンパイル & バイトコードセット
    auto csBlob = compiler.CompileShader(desc.csPath, desc.csEntry, desc.csModel);
    if (csBlob) {
        psoDesc.CS = { csBlob->GetBufferPointer(), csBlob->GetBufferSize() };
    }
    else {
        Debugger::Log("[Error] Failed to compile CS Shader: %s (%s)\n", desc.csPath.c_str(), desc.csEntry.c_str());
        return result;
    }

    // 4. PSOの生成
    result.rootSignature = desc.rootSignature;
    HRESULT hr = dev->CreateComputePipelineState(&psoDesc, IID_PPV_ARGS(&result.pipelineState));

    if (FAILED(hr)) {
        Debugger::Log("[Error] Failed to create Compute PSO: %s (%s)\n", desc.csPath.c_str(), desc.csEntry.c_str());
        return result;
    }

    // デバッグ用の名前設定
    std::string psoName = desc.csPath + " [" + desc.csEntry + "]";
    result.pipelineState->SetName(StringConverter::ToLPCWSTR(psoName).c_str());

    return result;
}