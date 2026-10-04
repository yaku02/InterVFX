#include "pch.h"
#include "VFXPass.h"
#include "Debugger.h"

void VFXPass::Init(const InitDesc& desc)
{
	CreatePassCB(desc.gpuResMgr);
	m_renderer = std::make_unique<VFXRenderer>();
	m_renderer->Init({ .dx12 = desc.dx12, .gpuResMgr = desc.gpuResMgr });

	m_updater = std::make_unique<VFXUpdater>();
	m_updater->Init({ .dx12 = desc.dx12, .gpuResMgr = desc.gpuResMgr });
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

	if (m_updater) {
		m_updater->Update({ desc.updateDesc });
	}
	if (m_renderer) {
		m_renderer->Render({ desc.renderDesc });
	}
}

