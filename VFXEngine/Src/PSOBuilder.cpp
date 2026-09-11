#include "pch.h"
#include "PSOBuilder.h"
#include "ShaderCompiler.h"
#include "Debugger.h"
#include "StringConverter.h"

PipelineObject PSOBuilder::CreateComputePSO(ID3D12Device* dev, const ComputePSODesc& desc)
{
	ShaderCompiler cs;

	auto shaderBlob = cs.CompileShader(desc.csPath, desc.csEntry, desc.csModel);
	D3D12_COMPUTE_PIPELINE_STATE_DESC psoDesc = {};
	psoDesc.pRootSignature = desc.rootSignature.Get();
	psoDesc.CS = { shaderBlob->GetBufferPointer(), shaderBlob->GetBufferSize() };

	PipelineObject pObj;
	pObj.rootSignature = desc.rootSignature.Get();
	auto hr = dev->CreateComputePipelineState(&psoDesc, IID_PPV_ARGS(&pObj.pipelineState));
	if (FAILED(hr)) {
		Debugger::Log("[Error] Failed to create Compute PSO: %s\n", desc.csPath.c_str());
	}
	pObj.pipelineState->SetName(StringConverter::ToLPCWSTR(desc.csEntry).c_str());

	return pObj;
}

PipelineObject PSOBuilder::CreateGraphicsPSO(ID3D12Device* dev, const GraphicsPSODesc& desc)
{
	if (desc.rootSignature == nullptr) {
		Debugger::Log("[Error] CreateGrahicsPSO-> rootSignature is null\n");
		return PipelineObject{};
	}

	ShaderCompiler compiler;

	auto vsBlob = compiler.CompileShader(desc.vsPath, desc.vsEntry, desc.vsModel);
	auto psBlob = compiler.CompileShader(desc.psPath, desc.psEntry, desc.psModel);

	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
	psoDesc.pRootSignature = desc.rootSignature.Get();

	// 2. シェーダーバイトコードのセット
	if (vsBlob) {
		psoDesc.VS = { vsBlob->GetBufferPointer(), vsBlob->GetBufferSize() };
	}
	if (psBlob) {
		psoDesc.PS = { psBlob->GetBufferPointer(), psBlob->GetBufferSize() };
	}

	// 3. ブレンドステートの設定
	psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	D3D12_RENDER_TARGET_BLEND_DESC& rtBlend = psoDesc.BlendState.RenderTarget[0];
	rtBlend.BlendEnable = TRUE;
	rtBlend.SrcBlend = D3D12_BLEND_SRC_ALPHA;
	rtBlend.BlendOp = D3D12_BLEND_OP_ADD;
	rtBlend.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

	if (desc.blendMode == BlendMode::Additive) {
		// 加算合成設定
		rtBlend.DestBlend = D3D12_BLEND_ONE;
		rtBlend.SrcBlendAlpha = D3D12_BLEND_ONE;
		rtBlend.DestBlendAlpha = D3D12_BLEND_ONE;
		rtBlend.BlendOpAlpha = D3D12_BLEND_OP_ADD;
	}
	else {
		// 通常のアルファ半透明設定
		rtBlend.DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
		rtBlend.SrcBlendAlpha = D3D12_BLEND_ONE;
		rtBlend.DestBlendAlpha = D3D12_BLEND_ZERO;
		rtBlend.BlendOpAlpha = D3D12_BLEND_OP_ADD;
	}

	// 4. ラスタライザーステートの設定 (カリングなし・裏表描画)
	psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE; // VFXは両面描画が多い

	// 5. デプスステンシルステートの設定 (Zテストあり / Z書き込み切り替え)
	psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	psoDesc.DepthStencilState.DepthEnable = (desc.dsvFormat != DXGI_FORMAT_UNKNOWN);
	psoDesc.DepthStencilState.DepthWriteMask = desc.isDepthWrite ? D3D12_DEPTH_WRITE_MASK_ALL : D3D12_DEPTH_WRITE_MASK_ZERO;
	psoDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;

	// 6. 入力レイアウト (Bindless や SV_VertexID 描画なら空でOK)
	psoDesc.InputLayout = { nullptr, 0 };

	// 7. トポロジ・レンダーターゲット設定
	psoDesc.PrimitiveTopologyType = desc.topologyType;
	psoDesc.NumRenderTargets = 1;
	psoDesc.RTVFormats[0] = desc.rtvFormat;
	psoDesc.DSVFormat = desc.dsvFormat;
	psoDesc.SampleMask = UINT_MAX;
	psoDesc.SampleDesc.Count = 1;

	// 8. PSOの生成
	PipelineObject pObj;
	pObj.rootSignature = desc.rootSignature;
	HRESULT hr = dev->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&pObj.pipelineState));
	pObj.pipelineState->SetName(StringConverter::ToLPCWSTR(desc.vsEntry + "/" + desc.psEntry).c_str());

	if (FAILED(hr)) {
		Debugger::Log("[Error] Failed to create Graphics PSO: %s / %s\n", desc.vsPath.c_str(), desc.psPath.c_str());
	}

	return pObj;
}
