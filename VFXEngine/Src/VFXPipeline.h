#pragma once
#include "PipelineStructs.h"

enum class GraphicsPSOType{
	Additive = 0,
	Alpha,
	PreviewAdditive,
	PreviewAlpha,
	Count
};

enum class ComputePSOType {
	BasicUpdate = 0,
	FroxelInject,
	Count,
};

class VFXPipeline
{
	template <typename T>
	using ComPtr = Microsoft::WRL::ComPtr<T>;
private:

	ComPtr<ID3D12Device> m_dev = nullptr;

	std::unordered_map<uint32_t, PipelineObject> m_computePSOs;
	std::unordered_map<uint32_t, PipelineObject> m_graphicsPSOs;

	HRESULT CreateComputePipelines();
	HRESULT CreateGraphicsPipelines();
public:

	void Init(ID3D12Device* dev) {
		m_dev = dev;

		CreateComputePipelines();
		CreateGraphicsPipelines();
	}

	void SetComputePSO(ID3D12GraphicsCommandList* cmdList, uint32_t psoID);
	void SetGraphicsPSO(ID3D12GraphicsCommandList* cmdList, uint32_t psoID);
};