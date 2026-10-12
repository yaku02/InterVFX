#pragma once
#include "ConstantBuffer.h"
#include "VFXRenderStage.h"
#include "VFXUpdateStage.h"
#include "VFXRespawnStage.h"
#include "VFXCopyArgsStage.h"

class Dx12Wrapper;
class GpuResourceManager;
class VFXPass {
private:
	std::unique_ptr<VFXRenderStage> m_renderStage = nullptr;
	std::unique_ptr<VFXUpdateStage> m_updateStage = nullptr;
	std::unique_ptr<VFXRespawnStage> m_respawnStage = nullptr;
	std::unique_ptr<VFXCopyArgsStage> m_copyArgsStage = nullptr;

	struct PassCBDesc {
		struct Param {
			float vfxPassNoise = 1.0f;
			float padding[3];
		}param{};
		struct DescIndices {
			float dummy[4];
		}descIndices{};
	}m_passCBDesc{};

	ConstantBuffer<PassCBDesc> m_passCB;
	HRESULT CreatePassCB(GpuResourceManager& gpuResMgr);

public:
	struct InitDesc {
		Dx12Wrapper& dx12;
		GpuResourceManager& gpuResMgr;
	};
	void Init(const InitDesc& desc);

	struct PassExecuteDesc {
		VFXRenderStage::RenderDesc renderDesc{};
		VFXUpdateStage::UpdateDesc updateDesc{};
		VFXRespawnStage::RespawnDesc respawnDesc{};
		VFXCopyArgsStage::ExeDesc copyArgsDesc{};
	};

	void Execute(const PassExecuteDesc& desc);
};