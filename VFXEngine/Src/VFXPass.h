#pragma once
#include "ConstantBuffer.h"
#include "VFXRenderer.h"
#include "VFXUpdater.h"

class Dx12Wrapper;
class GpuResourceManager;
class VFXPass {
private:
	std::unique_ptr<VFXRenderer> m_renderer = nullptr;
	std::unique_ptr<VFXUpdater> m_updater = nullptr;

	struct PassCBDesc {
		struct Param {
			float vfxPassNoise; // テスト用
			float padding[3];
		}param;
		struct DescIndices {
		}descIndices;
	}m_passCBDesc;

	ConstantBuffer<PassCBDesc> m_passCB;
	HRESULT CreatePassCB(GpuResourceManager& gpuResMgr);

public:
	struct InitDesc {
		Dx12Wrapper& dx12;
		GpuResourceManager& gpuResMgr;
	};
	void Init(const InitDesc& desc);

	struct PassExecuteDesc {
		VFXRenderer::RenderDesc renderDesc;
		VFXUpdater::UpdateDesc updateDesc;
	};

	void Execute(const PassExecuteDesc& desc);
};