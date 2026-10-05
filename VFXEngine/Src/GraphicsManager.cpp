#include "pch.h"
#include "GraphicsManager.h"
#include "Window.h"
#include "Dx12Wrapper.h"
#include "GpuResourceManager.h"
#include "TextureManager.h"

#include "VFXManager.h"

#include "Debugger.h"
#include "Timer.h"
#include "EditorCamera.h"
#include "SceneUI.h"

void GraphicsManager::Init(const InitDesc& desc)
{
	m_window = &desc.window;
	m_dx12 = &desc.dx12;
	m_gpuResMgr = &desc.gpuResMgr;
	m_texMgr = &desc.texMgr;

	float aspectRatio = static_cast<float>(desc.window.GetWindowWidth()) / static_cast<float>(desc.window.GetWindowHeight());
	EditorCamera::Init(DirectX::XMConvertToRadians(45.0f), aspectRatio, 0.1f, 1000.0f);

	CreateGlobalCB(desc.gpuResMgr);

	{
		m_vfxMgr = std::make_unique<VFXManager>();
		VFXManager::InitDesc vfxDesc{
			.window = desc.window,
			.dx12 = desc.dx12,
			.gpuResMgr = desc.gpuResMgr,
			.texMgr = desc.texMgr,
		};
		m_vfxMgr->Init(vfxDesc);
	}

	SceneUI::RegisterRenderCallback(
		[this]() -> void {
			return this->Execute();
		}
	);
}

HRESULT GraphicsManager::CreateGlobalCB(GpuResourceManager& gpuResMgr)
{
	m_globalCB = ConstantBuffer<GlobalCBDesc>::Create(gpuResMgr);

	if (m_globalCB.resource == nullptr || m_globalCB.mapData == nullptr || m_globalCB.descriptorIndex == UINT32_MAX)
	{
		Debugger::Log("[Error] Failed to create Global ConstantBuffer\n");
		return E_FAIL;
	}
	return S_OK;
}

void GraphicsManager::Execute()
{
	ID3D12DescriptorHeap* heaps[] = { m_gpuResMgr->GetDescHeap(HeapType::Visible).heap.Get()};
	m_dx12->GetCmdList()->SetDescriptorHeaps(1, heaps);

	float dt = Timer::GetGlobalDeltaTime();
	EditorCamera::Update(dt);

	m_globalCBDesc.param.globalDeltaTime = dt;
	m_globalCBDesc.param.viewProj = EditorCamera::GetViewProjFloat4x4();
	m_globalCB.Upload(m_globalCBDesc);

	m_vfxMgr->Execute(VFXManager::ExecuteDesc{ .grobalCBVIndex = m_globalCB.descriptorIndex });
}

void GraphicsManager::ShutDown()
{
	m_vfxMgr->ShutDown();
}