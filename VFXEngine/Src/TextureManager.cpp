#include "pch.h"
#include "TextureManager.h"
#include "Dx12Wrapper.h"
#include "GpuResourceManager.h"
#include "Debugger.h"

void TextureManager::Init(const InitDesc& initDesc)
{
	m_dx12 = &initDesc.dx12;
	m_gpuResMgr = &initDesc.gpuResMgr;

	CreateWhiteTex();
}

HRESULT TextureManager::CreateWhiteTex()
{
	const auto& dev = m_dx12->GetDevice().Get();

	uint32_t whitePixel = 0xFFFFFFFF; // 8個 (RGBAがすべて255)

	D3D12_HEAP_PROPERTIES dummyHeapProp = CD3DX12_HEAP_PROPERTIES(D3D12_CPU_PAGE_PROPERTY_WRITE_BACK, D3D12_MEMORY_POOL_L0);
	D3D12_RESOURCE_DESC dummyResDesc = CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_R8G8B8A8_UNORM, 1, 1);
	dummyResDesc.MipLevels = 1;

	auto hr = dev->CreateCommittedResource(
		&dummyHeapProp, D3D12_HEAP_FLAG_NONE, &dummyResDesc,
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, nullptr,
		IID_PPV_ARGS(&m_whiteTex.buffer));

	if (FAILED(hr)) {
		Debugger::Log("Creation whiteTexture failed\n");
		return E_FAIL;
	}

	m_whiteTex.buffer->WriteToSubresource(
		0, nullptr, &whitePixel,
		4, // 1行あたりのバイト数 (4バイト)
		4  // 全体のバイト数 (4バイト)
	);

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Texture2D.MipLevels = 1;

	D3D12_CPU_DESCRIPTOR_HANDLE visibleCpuHandle =
		m_gpuResMgr->AllocateDescriptor(&m_whiteTex.engineSrvHandle);

	dev->CreateShaderResourceView(m_whiteTex.buffer.Get(), &srvDesc, visibleCpuHandle);
	if (visibleCpuHandle.ptr != 0) {
		dev->CreateShaderResourceView(
			m_whiteTex.buffer.Get(), &srvDesc, visibleCpuHandle);
	}

	D3D12_CPU_DESCRIPTOR_HANDLE imguiCpuHandle =
		m_gpuResMgr->AllocateDescriptor(&m_whiteTex.imguiSrvHandle, nullptr, HeapType::ImGui);

	if (imguiCpuHandle.ptr != 0) {
		dev->CreateShaderResourceView(
			m_whiteTex.buffer.Get(), &srvDesc, imguiCpuHandle);
	}
	return S_OK;
}