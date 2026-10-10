#pragma once
#include "GraphicsPipeline.h"

class Dx12Wrapper;
class GpuResourceManager;
class VFXInstance;
class VFXAsset;
class VFXPreview;
class VFXRenderer {
public:
	struct InitDesc {
		Dx12Wrapper& dx12;
		GpuResourceManager& gpuResMgr;
	};
		
	struct RenderDesc {
		std::vector<std::shared_ptr<VFXInstance>>* instances = nullptr;
		std::unordered_map<uint32_t, std::shared_ptr<VFXAsset>>* assetsMap = nullptr;
		uint32_t globalCBVIndex = UINT32_MAX;
		uint32_t passCBVIndex = UINT32_MAX;
		VFXPreview* preview = nullptr;
	};

private:
	Dx12Wrapper* m_dx12 = nullptr;
	GpuResourceManager* m_gpuResMgr = nullptr;

	GraphicsPipeline m_additivePipeline;
	GraphicsPipeline m_alphaPipeline;

	void CreatePipeline(ID3D12Device* dev);
public:
	void Init(const InitDesc& desc);
	void Render(const RenderDesc& desc);
};