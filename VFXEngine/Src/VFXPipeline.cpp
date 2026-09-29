#include "pch.h"

#include "VFXPipeline.h"
#include "PSOBuilder.h"
#include "RootSignatureManager.h"
#include "Debugger.h"

HRESULT VFXPipeline::CreateGraphicsPipelines()
{
	const auto& rootSig = RootSignatureManager::GetRootSignature();

	// 共通パラメータ
	const auto& vsPath = "Engine/Shader/VFXVertexShader.hlsl";
	const auto& psPath = "Engine/Shader/VFXPixelShader.hlsl";
	const auto& vsEntry = "VFXVS";
	const auto& psEntry = "VFXPS";

	const DXGI_FORMAT rtvFormat = DXGI_FORMAT_R16G16B16A16_FLOAT;
	const DXGI_FORMAT previewRTVFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
	const DXGI_FORMAT dsvFormat = DXGI_FORMAT_D32_FLOAT;

	GraphicsPSODesc additive;
	additive.vsPath = vsPath;
	additive.psPath = psPath;
	additive.vsEntry = vsEntry;
	additive.psEntry = psEntry;
	additive.rootSignature = rootSig;
	additive.rtvFormat = rtvFormat;
	additive.isDepthWrite = false;
	additive.dsvFormat = dsvFormat;
	additive.topologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	additive.blendMode = BlendMode::Additive;
	m_graphicsPSOs[static_cast<uint32_t>(GraphicsPSOType::Additive)] = PSOBuilder::CreateGraphicsPSO(m_dev.Get(), additive);
	additive.rtvFormat = previewRTVFormat;
	m_graphicsPSOs[static_cast<uint32_t>(GraphicsPSOType::PreviewAdditive)] = PSOBuilder::CreateGraphicsPSO(m_dev.Get(), additive);

	GraphicsPSODesc alpha;
	alpha.vsPath = vsPath;
	alpha.psPath = psPath;
	alpha.vsEntry = vsEntry;
	alpha.psEntry = psEntry;
	alpha.rootSignature = rootSig;
	alpha.rtvFormat = rtvFormat;
	alpha.isDepthWrite = false;
	alpha.dsvFormat = dsvFormat;
	alpha.topologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	alpha.blendMode = BlendMode::Alpha;
	m_graphicsPSOs[static_cast<uint32_t>(GraphicsPSOType::Alpha)] = PSOBuilder::CreateGraphicsPSO(m_dev.Get(), alpha);
	alpha.rtvFormat = previewRTVFormat;
	m_graphicsPSOs[static_cast<uint32_t>(GraphicsPSOType::PreviewAlpha)] = PSOBuilder::CreateGraphicsPSO(m_dev.Get(), alpha);

	return S_OK;
}

HRESULT VFXPipeline::CreateComputePipelines()
{
	const auto& rootSig = RootSignatureManager::GetRootSignature();

	ComputePSODesc update;
	update.csPath = "Engine/Shader/VFXComputeShader.hlsl";
	update.csEntry = "UpdateCS";
	update.rootSignature = rootSig;
	m_computePSOs[static_cast<uint32_t>(ComputePSOType::BasicUpdate)] = PSOBuilder::CreateComputePSO(m_dev.Get(), update);

	ComputePSODesc inject;
	inject.csPath = "Engine/Shader/VFXComputeShader.hlsl";
	inject.csEntry = "FroxelInjectCS";
	inject.rootSignature = rootSig;
	m_computePSOs[static_cast<uint32_t>(ComputePSOType::FroxelInject)] = PSOBuilder::CreateComputePSO(m_dev.Get(), inject);

	return S_OK;
}

void VFXPipeline::SetGraphicsPSO(ID3D12GraphicsCommandList* cmdList, uint32_t psoID)
{
	auto it = m_graphicsPSOs.find(psoID);
	if (it == m_graphicsPSOs.end()) return;
	it->second.SetGraphicsPipeline(cmdList);
}

void VFXPipeline::SetComputePSO(ID3D12GraphicsCommandList* cmdList, uint32_t psoID)
{
	auto it = m_computePSOs.find(psoID);
	if (it == m_computePSOs.end()) return;
	it->second.SetComputePipeline(cmdList);
}