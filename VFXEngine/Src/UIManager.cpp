#include "pch.h"
#include "UIManager.h"
#include "Window.h"
#include "Dx12Wrapper.h"
#include "GpuResourceManager.h"
#include "BackBufferManager.h"
#include "SceneUI.h"
#include "LogStream.h"
#include "AssetUI.h"
#include "InstUI.h"
#include "VFXAssetEditor.h"
#include "VFXInstEditor.h"
#include "PreviewUI.h"

UIManager::UIManager() = default;
UIManager::~UIManager() = default;

void UIManager::Init(const InitDesc& initDesc)
{
	m_window = &initDesc.window;
	m_dx12 = &initDesc.dx12;
	m_gpuResMgr = &initDesc.gpuResMgr;
	m_backBufferMgr = &initDesc.backBufferMgr;
	m_vfxAssetEditor = initDesc.vfxAssetEditor;
	m_vfxInstEditor = initDesc.vfxInstEditor;

	{
		m_sceneUI = std::make_unique<SceneUI>();
		SceneUI::InitDesc sceneInit{
			.window = initDesc.window,
			.dx12 = initDesc.dx12,
			.gpuResMgr = initDesc.gpuResMgr
		};

		m_sceneUI->Setup(sceneInit);
	}
	{
		m_assetUI = std::make_unique<AssetUI>();
	}
	{
		m_instUI = std::make_unique<InstUI>();
	}
	{
		m_previewUI = std::make_unique<PreviewUI>();
		PreviewUI::InitDesc desc{
			.window = m_window,
			.dx12 = m_dx12,
			.gpuResMgr = m_gpuResMgr
		};
		m_previewUI->Init(desc);
	}
}

void UIManager::ShowUIs()
{
	if (ImGui::Begin("Asset Editor"))
	{
		// 左右2列のテーブルを作成
		if (ImGui::BeginTable("AssetEditorLayout", 2, ImGuiTableFlags_Resizable | ImGuiTableFlags_BordersInnerV))
		{
			ImGui::TableSetupColumn("PreviewColumn", ImGuiTableColumnFlags_WidthStretch, 0.6f);
			ImGui::TableSetupColumn("EditorColumn", ImGuiTableColumnFlags_WidthStretch, 0.4f);

			// --- 左列: Preview (スクロールさせず固定表示) ---
			ImGui::TableNextColumn();
			m_previewUI->ShowUI();

			// --- 右列: Asset Editor (内部でのみ縦スクロールを許可) ---
			ImGui::TableNextColumn();

			// 右列の利用可能領域いっぱいにスクロール可能な Child Window を作成
			ImGui::BeginChild("AssetEditorScrollRegion", ImGui::GetContentRegionAvail(), false);

			m_vfxAssetEditor->ShowUI(); // パラメータ群を描画

			ImGui::EndChild();

			ImGui::EndTable();
		}
	}
	ImGui::End();

	if (ImGui::Begin("Instance Editor"))
	{
		m_vfxInstEditor->ShowUI();
	}
	ImGui::End();

	m_sceneUI->ShowUI();

	m_assetUI->ShowUI();

	m_instUI->ShowUI();

	LogStream::Get().Draw("Console Log");
}

void UIManager::ExecuteRendering()
{
	const auto& cmdList = m_dx12->GetCmdList().Get();
	const auto& rtvHandle = m_backBufferMgr->GetCurrentBackBufferRtvHandle();

	m_sceneUI->Render();
	m_previewUI->Render();

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
	m_assetUI.reset();
	ImGui_ImplDX12_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();
}