#include "pch.h"
#include "SceneUI.h"
#include "Window.h"
#include "Dx12Wrapper.h"
#include "GpuResourceManager.h"

void SceneUI::Render()
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

	float clearColor[] = { 0.0f, 0.0f, 0.0f, 1.0f };
	cmdList->ClearRenderTargetView(m_rtvHandle, clearColor, 0, nullptr);

	// ★ 追加：ビューポートとシザー矩形（RECT）の設定
	D3D12_VIEWPORT viewport = {
		0.0f, 0.0f,
		static_cast<float>(m_window->GetWindowWidth()),
		static_cast<float>(m_window->GetWindowHeight()),
		0.0f, 1.0f
	};
	D3D12_RECT scissorRect = {
		0, 0,
		static_cast<LONG>(m_window->GetWindowWidth()),
		static_cast<LONG>(m_window->GetWindowHeight())
	};
	cmdList->RSSetViewports(1, &viewport);
	cmdList->RSSetScissorRects(1, &scissorRect);

	// 2. GraphicsManagerの処理を開始する（描画コマンドの発行）
	if (m_renderCallback) {
		m_renderCallback();
	}

	// 3. リソースバリア：RENDER_TARGET -> SHADER_RESOURCE (ImGui用)
	barrier = CD3DX12_RESOURCE_BARRIER::Transition(
		m_buffer.Get(),
		D3D12_RESOURCE_STATE_RENDER_TARGET,
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE
	);
	cmdList->ResourceBarrier(1, &barrier);
}

void SceneUI::ShowUI()
{
	ImGui::Begin("Scene");

	// 2. ウィンドウ内の描画可能エリア（表示サイズ）を取得
	ImVec2 viewportSize = ImGui::GetContentRegionAvail();

	// 3. サイズが有効な場合のみ描画
	if (viewportSize.x > 0.0f && viewportSize.y > 0.0f)
	{
		// GPU ハンドルの ptr (UINT64) を ImTextureID (void* または ImTextureID) にキャスト
		ImTextureID textureId = (ImTextureID)m_srvHandle.ptr;

		// ImGui にテクスチャと表示サイズを渡して描画
		ImGui::Image(textureId, viewportSize);
	}

	ImGui::End();
}

void SceneUI::Setup(InitDesc& initDesc)
{
	m_window = &initDesc.window;
	m_dx12 = &initDesc.dx12;
	m_gpuResMgr = &initDesc.gpuResMgr;

	CreateResource();
}

HRESULT SceneUI::CreateResource()
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

	float clearColor[] = { 0.0f, 0.0f, 0.0f, 1.0f };

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
