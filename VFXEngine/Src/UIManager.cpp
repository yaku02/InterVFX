#include "pch.h"
#include "UIManager.h"
#include "Window.h"
#include "Dx12Wrapper.h"
#include "GpuResourceManager.h"
#include "BackBufferManager.h"
#include "SceneUI.h"
#include "LogStream.h"

UIManager::UIManager() = default;
UIManager::~UIManager() = default;

void UIManager::Init(InitDesc& initDesc)
{
	m_window = &initDesc.window;
	m_dx12 = &initDesc.dx12;
	m_gpuResMgr = &initDesc.gpuResMgr;
	m_backBufferMgr = &initDesc.backBufferMgr;

	{
		m_sceneUI = std::make_unique<SceneUI>();
		SceneUI::InitDesc sceneInit{
			.window = initDesc.window,
			.dx12 = initDesc.dx12,
			.gpuResMgr = initDesc.gpuResMgr
		};

		m_sceneUI->Setup(sceneInit);
	}
}

void UIManager::ShowUIs()
{
	// UIの構築
	ImGui::Begin("Instance Window", nullptr);
	ImGui::Text("Hello, ImGui!");
	ImGui::End();

	// UIの構築
	ImGui::Begin("Editor Window", nullptr);
	ImGui::Text("Hello, ImGui!");
	ImGui::End();

	m_sceneUI->ShowUI();

	LogStream::Get().Draw("Console Log");
}

void UIManager::ExecuteRendering()
{
	const auto& cmdList = m_dx12->GetCmdList().Get();
	const auto& rtvHandle = m_backBufferMgr->GetCurrentBackBufferRtvHandle();

	m_sceneUI->RenderScene();

	RECT rect;
	GetClientRect(m_window->GetHWND(), &rect);

	ImGuiIO& io = ImGui::GetIO();
	io.DisplaySize = ImVec2(
		(float)(rect.right - rect.left),
		(float)(rect.bottom - rect.top)
	);

	ImGui_ImplDX12_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();
	
	ShowUIs();

	cmdList->OMSetRenderTargets(1, &rtvHandle, FALSE, nullptr);

	// ImGui の描画用 Descriptor Heap をセット
	ID3D12DescriptorHeap* heaps[] = { m_gpuResMgr->GetDescHeap(HeapType::ImGui).heap.Get() };
	cmdList->SetDescriptorHeaps(_countof(heaps), heaps);

	ImGui::Render();
	ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), cmdList);
}

void UIManager::ShutDown()
{
	m_sceneUI.reset();
	ImGui_ImplDX12_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();
}