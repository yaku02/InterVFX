#pragma once
#include "ConstantBuffer.h"
#include "ComputePipeline.h"
#include "VFXAsset.h"
#include "VFXInstance.h"

struct VFXPassIndices {
	uint32_t globalPassCBVIndex;
	uint32_t assetCBVIndex;
	uint32_t instanceCBVIndex;
};

class Dx12Wrapper;
class GpuResourceManager;
class VFXUpdater {
	template <typename T>
	using ComPtr = Microsoft::WRL::ComPtr<T>;

public:
	struct InitDesc {
		Dx12Wrapper& dx12;
		GpuResourceManager& gpuResMgr;
	};
	
	struct UpdateDesc {
		std::vector<std::shared_ptr<VFXInstance>>* instances = nullptr;
		std::unordered_map<uint32_t, std::shared_ptr<VFXAsset>>* assetsMap = nullptr;
		uint32_t globalCBVIndex = UINT32_MAX;
		uint32_t passCBVIndex = UINT32_MAX;
	};

private:
	GpuResourceManager* m_gpuResMgr = nullptr;
	Dx12Wrapper* m_dx12 = nullptr;

	ComputePipeline m_updatePipeline;

	void CreatePipeline(ID3D12Device* dev);
public:
	void Init(const InitDesc& desc);
	void Update(const UpdateDesc& desc);
};