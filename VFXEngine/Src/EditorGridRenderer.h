#pragma once
#include "GraphicsPipeline.h"

class GpuResourceManager;
class EditorGridRenderer {
public:
	struct InitDesc {
		ID3D12Device* device;
		GpuResourceManager& gpuResMgr;
	};

	struct RenderDesc {
		ID3D12GraphicsCommandList* cmdList;
		uint32_t globalCBVIndex;
	};

private:
	Microsoft::WRL::ComPtr<ID3D12Device> m_dev = nullptr;

	GpuResourceManager* m_gpuResMgr = nullptr;

	GraphicsPipeline m_pipeline;

	void CreatePipeline(ID3D12Device* dev);
public:
	void Init(const InitDesc& desc);
	void Render(const RenderDesc& desc);
};