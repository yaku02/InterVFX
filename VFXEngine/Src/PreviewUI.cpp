#include "pch.h"
#include "PreviewUI.h"
#include "Window.h"
#include "Dx12Wrapper.h"
#include "GpuResourceManager.h"

void PreviewUI::Init(const InitDesc& desc)
{
	m_window = desc.window;
	m_dx12 = desc.dx12;
	m_gpuResMgr = desc.gpuResMgr;

	CreateResource();
}

void PreviewUI::Render()
{
	const auto& cmdList = m_dx12->GetCmdList().Get();
	// 1. リソースバリア：SHADER_RESOURCE -> RENDER_TARGET
	D3D12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
		m_buffer.Get(),
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
		D3D12_RESOURCE_STATE_RENDER_TARGET
	);
	cmdList->ResourceBarrier(1, &barrier);

	cmdList->OMSetRenderTargets(1, &m_rtvHandle, FALSE, nullptr);

	float clearColor[] = { 0.118f, 0.565f, 1.0f, 1.0f };
	cmdList->ClearRenderTargetView(m_rtvHandle, clearColor, 0, nullptr);

	// -------------------------------------------------------------
	// ※ ここにモデルやメッシュの DrawInstanced 等の描画処理が入ります
	// -------------------------------------------------------------

	// 3. リソースバリア：RENDER_TARGET -> SHADER_RESOURCE (ImGui用)
	barrier = CD3DX12_RESOURCE_BARRIER::Transition(
		m_buffer.Get(),
		D3D12_RESOURCE_STATE_RENDER_TARGET,
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE
	);
	cmdList->ResourceBarrier(1, &barrier);
}


void PreviewUI::ShowUI()
{
	ImVec2 availSize = ImGui::GetContentRegionAvail();
	ImGuiWindowFlags childFlags = ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse;

	if (ImGui::BeginChild("PreviewViewport", availSize, false, childFlags))
	{
		// --- 1. 中央揃えタイトルの描画 ---
		const char* titleText = "PREVIEW";

		// テキスト自体の横幅を計算
		float titleWidth = ImGui::CalcTextSize(titleText).x;

		// (コンテンツ全体の幅 - テキスト幅) / 2 で中央のX座標を算出
		float centeredPosX = (ImGui::GetContentRegionAvail().x - titleWidth) * 0.5f;
		if (centeredPosX > 0.0f)
		{
			ImGui::SetCursorPosX(centeredPosX);
		}

		ImGui::TextColored(ImVec4(1.0f, 1.0f, 1.0f, 1.0f), "%s", titleText);

		// --- 2. 下部に細いセパレーター（区切り線）を挿入 ---
		ImGui::Spacing();
		ImGui::Separator();
		ImGui::Spacing();

		// --- 3. 残りの領域で画像を描画 ---
		ImVec2 viewportSize = ImGui::GetContentRegionAvail();

		if (viewportSize.x > 0.0f && viewportSize.y > 0.0f)
		{
			if (m_srvHandle.ptr != 0)
			{
				ImTextureID textureId = (ImTextureID)m_srvHandle.ptr;
				ImGui::Image(textureId, viewportSize);
			}
			else
			{
				// 未設定時のテキストも中央揃えにする場合
				const char* noTexText = "No Preview Texture Set.";
				float noTexWidth = ImGui::CalcTextSize(noTexText).x;
				float noTexPosX = (viewportSize.x - noTexWidth) * 0.5f;
				if (noTexPosX > 0.0f) ImGui::SetCursorPosX(noTexPosX);

				ImGui::TextDisabled("%s", noTexText);
			}
		}
	}
	ImGui::EndChild();
}
HRESULT PreviewUI::CreateResource()
{
	const auto& dev = m_dx12->GetDevice().Get();
	const auto& window_width = m_window->GetWindowWidth();
	const auto& window_height = m_window->GetWindowHeight();

	D3D12_HEAP_PROPERTIES heapProp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);

	D3D12_RESOURCE_DESC resDesc = CD3DX12_RESOURCE_DESC::Tex2D(
		DXGI_FORMAT_R16G16B16A16_FLOAT,
		window_width,
		window_height,
		1,
		1,
		1,
		0,
		D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET);

	float clearColor[] = { 0.118f, 0.565f, 1.0f, 1.0f };

	D3D12_CLEAR_VALUE cv = CD3DX12_CLEAR_VALUE(
		DXGI_FORMAT_R16G16B16A16_FLOAT, clearColor);

	dev->CreateCommittedResource(
		&heapProp,
		D3D12_HEAP_FLAG_NONE,
		&resDesc,
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
		&cv,
		IID_PPV_ARGS(&m_buffer));

	m_rtvHandle = m_gpuResMgr->AllocateDescriptor(nullptr, nullptr, HeapType::Rtv);
	dev->CreateRenderTargetView(m_buffer.Get(), nullptr, m_rtvHandle);

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Texture2D.MipLevels = 1;

	D3D12_CPU_DESCRIPTOR_HANDLE srvHandle =
		m_gpuResMgr->AllocateDescriptor(&m_srvHandle, nullptr, HeapType::ImGui);

	dev->CreateShaderResourceView(m_buffer.Get(), &srvDesc, srvHandle);

	return S_OK;
}

void PreviewUI::RegisterRenderingCallback(InstType type, RenderingCallback callback)
{
	m_renderingCallbacks[type] = callback;
}