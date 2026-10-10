#include "pch.h"
#include "GraphicsManager.h"
#include "Window.h"
#include "Dx12Wrapper.h"
#include "GpuResourceManager.h"
#include "TextureManager.h"

#include "VFXManager.h"
#include "EditorGridPass.h"
#include "Debugger.h"
#include "Timer.h"
#include "EditorCamera.h"
#include "SceneUI.h"
#include "PreviewUI.h"

GraphicsManager::GraphicsManager() = default;
GraphicsManager::~GraphicsManager() = default;

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
		VFXManager::InitDesc initDesc{
			.window = desc.window,
			.dx12 = desc.dx12,
			.gpuResMgr = desc.gpuResMgr,
			.texMgr = desc.texMgr,
		};
		m_vfxMgr->Init(initDesc);
	}

	{
		m_editorGridPass = std::make_unique<EditorGridPass>();
		EditorGridPass::InitDesc initDesc{
			.dev = desc.dx12.GetDevice().Get(),
			.gpuResMgr = desc.gpuResMgr
		};
		m_editorGridPass->Init(initDesc);
	}

	SceneUI::RegisterRenderCallback(
		[this]() -> void {
			return this->Execute(RenderMode::Scene);
		}
	);

	PreviewUI::RegisterRenderCallback(
		[this]() -> void {
			return this->Execute(RenderMode::PreviewOnly);
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

void GraphicsManager::Execute(RenderMode mode)
{
	ID3D12DescriptorHeap* heaps[] = { m_gpuResMgr->GetDescHeap(HeapType::Visible).heap.Get() };
	m_dx12->GetCmdList()->SetDescriptorHeaps(1, heaps);

	float dt = Timer::GetGlobalDeltaTime();
	EditorCamera::Update(dt);

	// 1. ŠeŽíƒpƒ‰ƒ[ƒ^‚ÌŽæ“¾
	DirectX::XMMATRIX viewProj = EditorCamera::GetViewProjMatrix();

	// yC³zs—ñŽ®‚ðŽó‚¯Žæ‚é•Ï”(det)‚ð—pˆÓ‚µ‚Ä‹ts—ñ‚ðŒvŽZ
	DirectX::XMVECTOR det;
	DirectX::XMMATRIX invViewProj = DirectX::XMMatrixInverse(&det, viewProj);

	DirectX::XMFLOAT3 cameraPos = EditorCamera::GetPosition();

	// 2. GlobalCBDesc ‚Ö‚ÌŠi”[ (“]’uˆ—‚Í‚±‚ê‚Åƒoƒbƒ`ƒŠOK‚Å‚·)
	m_globalCBDesc.param.globalDeltaTime = dt;
	DirectX::XMStoreFloat4x4(&m_globalCBDesc.param.viewProj, DirectX::XMMatrixTranspose(viewProj));
	DirectX::XMStoreFloat4x4(&m_globalCBDesc.param.invViewProj, DirectX::XMMatrixTranspose(invViewProj));
	m_globalCBDesc.param.cameraPos = cameraPos;

	// 3. GPU (ConstantBuffer) ‚ÖƒAƒbƒvƒ[ƒh
	m_globalCB.Upload(m_globalCBDesc);

	{
		EditorGridRenderer::RenderDesc renDesc{
			.cmdList = m_dx12->GetCmdList().Get(),
			.globalCBVIndex = m_globalCB.descriptorIndex
		};
		m_editorGridPass->Execute(EditorGridPass::PassExecuteDesc{ .renderDesc = renDesc,});
	}

	m_vfxMgr->Execute(VFXManager::ExecuteDesc{ .grobalCBVIndex = m_globalCB.descriptorIndex, .mode = mode });

}

void GraphicsManager::ShutDown()
{
	m_vfxMgr->ShutDown();
}

VFXManager* GraphicsManager::GetVFXMgr() { return m_vfxMgr.get(); }
EditorGridPass* GraphicsManager::GetEditorGridPass() { return m_editorGridPass.get(); }

