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

Texture TextureManager::LoadPNG(const std::string& filePath)
{
	Texture result{};

	const auto& dev = m_dx12->GetDevice().Get();
	const auto& cmdList = m_dx12->GetCmdList().Get();

	std::wstring wFilePath(filePath.begin(), filePath.end());

	std::unique_ptr<uint8_t[]> wicData;
	D3D12_SUBRESOURCE_DATA subresource;

	// 1. LoadWICTextureFromFileEx を使用して初期状態を COPY_DEST に指定
	HRESULT hr = DirectX::LoadWICTextureFromFileEx(
		dev,                        // ID3D12Device*
		wFilePath.c_str(),          // const wchar_t* szFileName
		0,                          // size_t maxsize (0で制限なし)
		D3D12_RESOURCE_FLAG_NONE,   // D3D12_RESOURCE_FLAGS resFlags
		DirectX::WIC_LOADER_DEFAULT,// DirectX::WIC_LOADER_FLAGS loadFlags
		result.buffer.GetAddressOf(),// ID3D12Resource** ppResource
		wicData,                    // std::unique_ptr<uint8_t[]>& wicData
		subresource                 // D3D12_SUBRESOURCE_DATA& subresource
	);

	if (FAILED(hr)) {
		Debugger::Log(("Failed to load: " + filePath + "\n").c_str());
		return result;
	}

	// 2. アップロードバッファの作成
	UINT64 bufferSize = GetRequiredIntermediateSize(result.buffer.Get(), 0, 1);

	D3D12_HEAP_PROPERTIES heapProp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	D3D12_RESOURCE_DESC resDesc = CD3DX12_RESOURCE_DESC::Buffer(bufferSize);

	ComPtr<ID3D12Resource> uploadBuffer = nullptr;

	hr = dev->CreateCommittedResource(
		&heapProp,
		D3D12_HEAP_FLAG_NONE,
		&resDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&uploadBuffer));

	if (FAILED(hr)) {
		Debugger::Log("creation Texture uploadBuffer failed\n");
		return result;
	}

	// 3. テクスチャデータの転送
	UpdateSubresources(
		cmdList,
		result.buffer.Get(),
		uploadBuffer.Get(),
		0, 0, 1,
		&subresource
	);

	m_tempUploadBuffers.push_back(uploadBuffer);

	// 4. リソースバリア（COPY_DEST -> PIXEL_SHADER_RESOURCE）
	D3D12_RESOURCE_BARRIER barrier = {};
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrier.Transition.pResource = result.buffer.Get();
	barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
	barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;

	cmdList->ResourceBarrier(1, &barrier);

	// 5. SRV (Shader Resource View) の設定
	D3D12_RESOURCE_DESC texDesc = result.buffer->GetDesc();

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

	DXGI_FORMAT srvFormat = texDesc.Format;
	if (srvFormat == DXGI_FORMAT_R8G8B8A8_TYPELESS) {
		srvFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
	}
	else if (srvFormat == DXGI_FORMAT_B8G8R8A8_TYPELESS) {
		srvFormat = DXGI_FORMAT_B8G8R8A8_UNORM;
	}
	else if (srvFormat == DXGI_FORMAT_UNKNOWN) {
		srvFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
	}
	srvDesc.Format = srvFormat;

	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MipLevels = texDesc.MipLevels;
	srvDesc.Texture2D.MostDetailedMip = 0;
	srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;

	// エンジン用 SRV の書き込み
	D3D12_CPU_DESCRIPTOR_HANDLE engineCpuHandle =
		m_gpuResMgr->AllocateDescriptor(&result.engineSrvHandle);
	if (engineCpuHandle.ptr != 0) {
		dev->CreateShaderResourceView(result.buffer.Get(), &srvDesc, engineCpuHandle);
	}

	// ImGui用 SRV の書き込み
	D3D12_CPU_DESCRIPTOR_HANDLE imguiCpuHandle =
		m_gpuResMgr->AllocateDescriptor(&result.imguiSrvHandle, nullptr, HeapType::ImGui);
	if (imguiCpuHandle.ptr != 0) {
		dev->CreateShaderResourceView(result.buffer.Get(), &srvDesc, imguiCpuHandle);
	}
	
	if (result.imguiSrvHandle.ptr == 0) {
		Debugger::Log(("Failed to load: " + filePath + "\n").c_str());
		return result;

	}
	Debugger::Log(("Loaded PNG file: " + filePath + "\n").c_str());
	return result;
}