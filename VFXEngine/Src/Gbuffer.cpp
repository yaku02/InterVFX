#include "pch.h"
#include "Gbuffer.h"
#include "GpuResourceManager.h"

HRESULT Gbuffer::CreateResource(ID3D12Device* dev, GpuResourceManager& gpuResMgr, int window_width, int window_height)
{
	D3D12_HEAP_PROPERTIES heapProp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);

	for (uint32_t idx = 0; idx < m_bufferCount; ++idx)
	{
		// 1. G-Buffer のフォーマット決定
		DXGI_FORMAT format = DXGI_FORMAT_UNKNOWN;
		if (idx == 0)      format = DXGI_FORMAT_R8G8B8A8_UNORM;     // 0: アルベド
		else if (idx == 1) format = DXGI_FORMAT_R16G16B16A16_FLOAT; // 1: 法線
		else               format = DXGI_FORMAT_R32G32B32A32_FLOAT; // 2: ワールド座標

		D3D12_RESOURCE_DESC resDesc = CD3DX12_RESOURCE_DESC::Tex2D(
			format,
			window_width,
			window_height,
			1, 1, 1, 0,
			D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET
		);

		float clearColor[] = { 0.0f, 0.0f, 0.0f, 1.0f };
		D3D12_CLEAR_VALUE cv = CD3DX12_CLEAR_VALUE(format, clearColor);

		// 2. リソースの作成
		HRESULT result = dev->CreateCommittedResource(
			&heapProp,
			D3D12_HEAP_FLAG_NONE,
			&resDesc,
			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, // 初期状態は読み取り専用
			&cv,
			IID_PPV_ARGS(&m_buffers[idx])
		);

		if (FAILED(result)) {
			OutputDebugStringA("Creation Gbuffers failed\n");
			return result;
		}

		// 3. RTV (RenderTargetView) の動的確保と作成
		// RTVマネージャー経由でCPUハンドルを取得（※RTV用のマネージャーがある前提）
		m_rtvHandles[idx] = gpuResMgr.AllocateDescriptor(nullptr, nullptr);

		dev->CreateRenderTargetView(
			m_buffers[idx].Get(),
			nullptr,
			m_rtvHandles[idx]
		);

		// 4. SRV (ShaderResourceView) の動的確保と作成
		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
		srvDesc.Format = format;
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srvDesc.Texture2D.MipLevels = 1;

		// 第1引数に &m_srvHandle[idx] を渡すことで、GPUハンドルを格納
		D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle =
			gpuResMgr.AllocateDescriptor(&m_srvHandles[idx], nullptr);

		dev->CreateShaderResourceView(
			m_buffers[idx].Get(),
			&srvDesc,
			cpuHandle
		);
	}

	return S_OK;
}

void Gbuffer::SetRenderTargetWithDepth(ID3D12GraphicsCommandList* cmdList, D3D12_CPU_DESCRIPTOR_HANDLE& dsvHandle)
{
	// Gバッファをレンダーターゲット状態へ遷移
	D3D12_RESOURCE_BARRIER barriers[3] = {};
	for (int i = 0; i < m_bufferCount; ++i) {
		barriers[i] = CD3DX12_RESOURCE_BARRIER::Transition(
			m_buffers[i].Get(),
			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
			D3D12_RESOURCE_STATE_RENDER_TARGET
		);
	}

	cmdList->ResourceBarrier(static_cast<UINT>(m_bufferCount), barriers);

	// Gバッファをセット
	cmdList->OMSetRenderTargets(static_cast<UINT>(m_bufferCount), m_rtvHandles, FALSE, &dsvHandle);

	// Gバッファをクリア
	float clearColor[] = { 0.0f, 0.0f, 0.0f, 1.0f };
	for (int i = 0; i < m_bufferCount; ++i) {
		cmdList->ClearRenderTargetView(m_rtvHandles[i], clearColor, 0, nullptr);
	}

	// 深度をクリア
	cmdList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
}