#include "pch.h"
#include "VFXPass.h"
#include "Debugger.h"

void VFXPass::Init(const InitDesc& desc)
{
	CreatePassCB(desc.gpuResMgr);
	m_renderStage = std::make_unique<VFXRenderStage>();
	m_renderStage->Init({ .dx12 = desc.dx12, .gpuResMgr = desc.gpuResMgr });

	m_updateStage = std::make_unique<VFXUpdateStage>();
	m_updateStage->Init({ .dx12 = desc.dx12, .gpuResMgr = desc.gpuResMgr });

	m_respawnStage = std::make_unique<VFXRespawnStage>();
	m_respawnStage->Init({ .dx12 = desc.dx12, .gpuResMgr = desc.gpuResMgr });

	m_copyArgsStage = std::make_unique<VFXCopyArgsStage>();
	m_copyArgsStage->Init({ .dx12 = desc.dx12, .gpuResMgr = desc.gpuResMgr });
}

HRESULT VFXPass::CreatePassCB(GpuResourceManager& gpuResMgr)
{
	m_passCB = ConstantBuffer<PassCBDesc>::Create(gpuResMgr);

	if (m_passCB.resource == nullptr || m_passCB.mapData == nullptr || m_passCB.descriptorIndex == UINT32_MAX)
	{
		Debugger::Log("[Error] Failed to create Pass ConstantBuffer\n");
		return E_FAIL;
	}
	return S_OK;
}

void VFXPass::Execute(const PassExecuteDesc& desc)
{
	m_passCBDesc.param.vfxPassNoise = 0.75f;
	m_passCB.Upload(m_passCBDesc);

	auto updateDesc = desc.updateDesc;
	updateDesc.passCBVIndex = m_passCB.descriptorIndex;

	auto renderDesc = desc.renderDesc;
	renderDesc.passCBVIndex = m_passCB.descriptorIndex;

	if (m_respawnStage) {
		m_respawnStage->Respawn({ desc.respawnDesc });
	}
	if (m_updateStage) {
		m_updateStage->Update({ desc.updateDesc });
	}
	if (m_copyArgsStage) {
		m_copyArgsStage->CopyArgs({ desc.copyArgsDesc });
	}
	if (m_renderStage) {
		m_renderStage->Render({ desc.renderDesc });
	}
}

