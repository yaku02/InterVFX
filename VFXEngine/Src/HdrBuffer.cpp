#include "pch.h"
#include "HdrBuffer.h"
#include "GpuResourceManager.h"

HRESULT HdrBuffer::CreateResource(ID3D12Device* dev, GpuResourceManager& gpuResMgr, int window_width, int window_height)
{
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

	m_rtvHandle = gpuResMgr.AllocateDescriptor(nullptr, nullptr);
	dev->CreateRenderTargetView(m_buffer.Get(), nullptr, m_rtvHandle);

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Texture2D.MipLevels = 1;

	D3D12_CPU_DESCRIPTOR_HANDLE srvHandle =
		gpuResMgr.AllocateDescriptor(&m_srvHandle, &m_srvIndex);

	dev->CreateShaderResourceView(m_buffer.Get(), &srvDesc, srvHandle);

	return S_OK;
}

void HdrBuffer::SetRenderTargetWithDepth(ID3D12GraphicsCommandList* cmdList, D3D12_CPU_DESCRIPTOR_HANDLE& dsvHandle) {
	cmdList->OMSetRenderTargets(1, &m_rtvHandle, FALSE, &dsvHandle);
}