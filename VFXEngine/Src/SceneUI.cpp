#include "pch.h"
#include "SceneUI.h"
#include "Window.h"
#include "Dx12Wrapper.h"
#include "GpuResourceManager.h"

void SceneUI::Render()
{
	if (m_needResize)
	{
		ResizeBuffers(m_pendingWidth, m_pendingHeight);
		m_needResize = false;
	}

	if (!m_colorBuffer.resource || m_bufferWidth == 0 || m_bufferHeight == 0) return;

	const auto& cmdList = m_dx12->GetCmdList().Get();

	// 1. リソースバリア：SHADER_RESOURCE -> RENDER_TARGET
	D3D12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
		m_colorBuffer.resource.Get(),
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
		D3D12_RESOURCE_STATE_RENDER_TARGET
	);
	cmdList->ResourceBarrier(1, &barrier);

	float clearColor[] = { 0.0f, 0.0f, 0.0f, 1.0f };
	cmdList->ClearRenderTargetView(m_colorBuffer.rtvHandle, clearColor, 0, nullptr);
	cmdList->ClearDepthStencilView(m_depthBuffer.dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

	cmdList->OMSetRenderTargets(1, &m_colorBuffer.rtvHandle, FALSE, &m_depthBuffer.dsvHandle);

	// ★ 追加：ビューポートとシザー矩形（RECT）の設定
	D3D12_VIEWPORT viewport = {
			0.0f, 0.0f,
			static_cast<float>(m_bufferWidth),
			static_cast<float>(m_bufferHeight),
			0.0f, 1.0f
	};
	D3D12_RECT scissorRect = {
		0, 0,
		static_cast<LONG>(m_bufferWidth),
		static_cast<LONG>(m_bufferHeight)
	};
	cmdList->RSSetViewports(1, &viewport);
	cmdList->RSSetScissorRects(1, &scissorRect);

	if (m_renderCallback) {
		m_renderCallback();
	}

	// 3. リソースバリア：RENDER_TARGET -> SHADER_RESOURCE (ImGui用)
	barrier = CD3DX12_RESOURCE_BARRIER::Transition(
		m_colorBuffer.resource.Get(),
		D3D12_RESOURCE_STATE_RENDER_TARGET,
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE
	);
	cmdList->ResourceBarrier(1, &barrier);
}

void SceneUI::ShowUI()
{
	ImGui::Begin("Scene");

	ImVec2 viewportSize = ImGui::GetContentRegionAvail();
	uint32_t newWidth = static_cast<uint32_t>(viewportSize.x);
	uint32_t newHeight = static_cast<uint32_t>(viewportSize.y);

	if (newWidth > 0 && newHeight > 0)
	{
		if (newWidth != m_bufferWidth || newHeight != m_bufferHeight)
		{
			// ★ その場で ResizeBuffers を呼ばず、フラグと新しいサイズを保持するだけにする
			m_needResize = true;
			m_pendingWidth = newWidth;
			m_pendingHeight = newHeight;
		}

		// 初回などでリソースが存在する場合のみ ImGui::Image を呼び出す
		if (m_colorBuffer.srvHandle.ptr != 0)
		{
			ImTextureID textureId = (ImTextureID)m_colorBuffer.srvHandle.ptr;
			ImGui::Image(textureId, viewportSize);
		}
	}

	ImGui::End();
}

void SceneUI::Init(InitDesc& initDesc)
{
	m_window = &initDesc.window;
	m_dx12 = &initDesc.dx12;
	m_gpuResMgr = &initDesc.gpuResMgr;
}

HRESULT SceneUI::CreateColorBuffer(uint32_t width, uint32_t height)
{
	const auto& dev = m_dx12->GetDevice().Get();

	D3D12_HEAP_PROPERTIES heapProp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);

	D3D12_RESOURCE_DESC resDesc = CD3DX12_RESOURCE_DESC::Tex2D(
		DXGI_FORMAT_R16G16B16A16_FLOAT,
		width,  // ← 引数の幅を使用
		height, // ← 引数の高さを使用
		1, 1, 1, 0,
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
		IID_PPV_ARGS(&m_colorBuffer.resource));

	m_colorBuffer.rtvHandle = m_gpuResMgr->AllocateDescriptor(nullptr, nullptr, HeapType::Rtv);
	dev->CreateRenderTargetView(m_colorBuffer.resource.Get(), nullptr, m_colorBuffer.rtvHandle);

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Texture2D.MipLevels = 1;

	D3D12_CPU_DESCRIPTOR_HANDLE srvHandle =
		m_gpuResMgr->AllocateDescriptor(&m_colorBuffer.srvHandle, nullptr, HeapType::ImGui);

	dev->CreateShaderResourceView(m_colorBuffer.resource.Get(), &srvDesc, srvHandle);

	return S_OK;
}

HRESULT SceneUI::CreateDepthBuffer(uint32_t width, uint32_t height)
{
	const auto& dev = m_dx12->GetDevice().Get();

	D3D12_HEAP_PROPERTIES heapProp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);

	D3D12_RESOURCE_DESC resDesc = CD3DX12_RESOURCE_DESC::Tex2D(
		DXGI_FORMAT_D32_FLOAT,
		width,  // ← 引数の幅を使用
		height, // ← 引数の高さを使用
		1, 1, 1, 0,
		D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL);

	// 3. クリア値設定（深度の初期値 1.0f）
	D3D12_CLEAR_VALUE cv = CD3DX12_CLEAR_VALUE(
		DXGI_FORMAT_D32_FLOAT,
		1.0f,  // depth
		0      // stencil
	);

	// 4. リソースの作成 (初期状態は DEPTH_WRITE)
	HRESULT hr = dev->CreateCommittedResource(
		&heapProp,
		D3D12_HEAP_FLAG_NONE,
		&resDesc,
		D3D12_RESOURCE_STATE_DEPTH_WRITE,
		&cv,
		IID_PPV_ARGS(&m_depthBuffer.resource));

	if (FAILED(hr))
	{
		Debugger::Log("[Error] Failed to create Depth Buffer resource.\n");
		return hr;
	}

	// 5. DSV 用のディスクリプタを割り当ててビューを作成
	// ※ HeapType::Dsv はご自身の GpuResourceManager の定義に合わせて調整してください
	m_depthBuffer.dsvHandle = m_gpuResMgr->AllocateDescriptor(nullptr, nullptr, HeapType::Dsv);

	D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
	dsvDesc.Format = DXGI_FORMAT_D32_FLOAT;
	dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
	dsvDesc.Flags = D3D12_DSV_FLAG_NONE;

	dev->CreateDepthStencilView(m_depthBuffer.resource.Get(), &dsvDesc, m_depthBuffer.dsvHandle);

	return S_OK;
}

void SceneUI::ResizeBuffers(uint32_t width, uint32_t height)
{
	// GPU の処理完了（WaitGPU）が必要な場合はここまたは呼び出し前で行う
	m_dx12->FlushCommandQueue(); // コマンドリスト実行中にリソース破棄されるのを防ぐ

	m_bufferWidth = width;
	m_bufferHeight = height;

	// 既存リソースの破棄（ComPtr なので Reset または代入で OK）
	m_colorBuffer.resource.Reset();
	m_depthBuffer.resource.Reset();

	// 再作成（※ Descriptor の解放・再利用ロジックがある場合は必要に応じて処理）
	CreateColorBuffer(m_bufferWidth, m_bufferHeight);
	CreateDepthBuffer(m_bufferWidth, m_bufferHeight);
}