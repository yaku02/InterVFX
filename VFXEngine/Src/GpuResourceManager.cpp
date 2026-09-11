#include "pch.h"
#include "GpuResourceManager.h"
#include "GpuResourceStructs.h"
#include "Debugger.h"
#include "Window.h"

void GpuResourceManager::SetUpDescHeap(HWND hwnd)
{
	// B. ディスクリプタヒープの作成 (全員分まとめて確保)
	{
		D3D12_DESCRIPTOR_HEAP_DESC descHeapDesc = {};
		descHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;

		descHeapDesc.NumDescriptors = 1000000;
		descHeapDesc.NodeMask = {};
		descHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

		auto result = m_dev->CreateDescriptorHeap(&descHeapDesc, IID_PPV_ARGS(&m_visibleHeap.heap));
		if (FAILED(result)) { return; }

		IncSize = m_dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		// ヒープの先頭ハンドルを取得
		m_visibleHeap.cpuHandle = m_visibleHeap.heap->GetCPUDescriptorHandleForHeapStart();
		m_visibleHeap.gpuHandle = m_visibleHeap.heap->GetGPUDescriptorHandleForHeapStart();
	}

	// NonShaderVisibleなヒープ作成
	{
		D3D12_DESCRIPTOR_HEAP_DESC descHeapDesc = {};
		descHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;

		descHeapDesc.NumDescriptors = MAX_DESC_COUNT;

		descHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

		auto result = m_dev->CreateDescriptorHeap(&descHeapDesc, IID_PPV_ARGS(&m_nonVisibleHeap.heap));
		if (FAILED(result)) { return; }

		m_nonVisibleHeap.cpuHandle = m_nonVisibleHeap.heap->GetCPUDescriptorHandleForHeapStart();
		m_nonVisibleHeap.gpuHandle = { 0 };
	}

	// RtvHeap作成
	{
		D3D12_DESCRIPTOR_HEAP_DESC descHeapDesc = {};
		descHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
		descHeapDesc.NumDescriptors = MAX_DESC_COUNT;
		descHeapDesc.NodeMask = 0;
		descHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

		auto result = m_dev->CreateDescriptorHeap(&descHeapDesc, IID_PPV_ARGS(&m_rtvHeap.heap));
		if (FAILED(result)) { return; }

		// RTV用の正しいインクリメントサイズを取得
		RtvIncSize = m_dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

		// ヒープの先頭CPUハンドルのみ取得（GPUハンドルはRTVでは使用しない）
		m_rtvHeap.cpuHandle = m_rtvHeap.heap->GetCPUDescriptorHandleForHeapStart();
		m_rtvHeap.gpuHandle = { 0 }; // 無効化または設定しない
	}

	// ImGui用ヒープ作成
	{
		D3D12_DESCRIPTOR_HEAP_DESC dhDesc = {};
		dhDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
		dhDesc.NumDescriptors = MAX_DESC_COUNT;
		dhDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
		m_dev->CreateDescriptorHeap(&dhDesc, IID_PPV_ARGS(&m_imguiHeap.heap));

		m_imguiHeap.cpuHandle = m_imguiHeap.heap->GetCPUDescriptorHandleForHeapStart();
		m_imguiHeap.gpuHandle = m_imguiHeap.heap->GetGPUDescriptorHandleForHeapStart();

		// セットアップ
		IMGUI_CHECKVERSION();
		ImGui::CreateContext();
		// 2. CPU側でフォントをビルド
		ImGuiIO& io = ImGui::GetIO();
		unsigned char* pixels;
		int width, height;
		io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);
		// ↑ これを呼ぶと内部で TexIsBuilt = true になる

		// 3. その後で Init
		ImGui_ImplWin32_Init(hwnd);


		ImGui_ImplDX12_Init(
			m_dev.Get(),
			3,
			DXGI_FORMAT_R8G8B8A8_UNORM,
			m_imguiHeap.heap.Get(),
			m_imguiHeap.heap->GetCPUDescriptorHandleForHeapStart(),
			m_imguiHeap.heap->GetGPUDescriptorHandleForHeapStart()
		);
		// ImGui が Index 0 を使用するため、アロケータの開始位置を 1 に進める
		m_imguiHeap.currentIdx = 1;
		ImGui_ImplDX12_CreateDeviceObjects();
	}
}

D3D12_CPU_DESCRIPTOR_HANDLE GpuResourceManager::AllocateDescriptor(D3D12_GPU_DESCRIPTOR_HANDLE * outGpuHandle, uint32_t* outIndex, HeapType heapType)
{
	uint32_t targetIdx = 0;

	auto& targetDescHeap = GetDescHeap(heapType);

	// もし過去に返却された空きインデックスがあれば、そこを再利用する
	if (!freeDescIndices.empty())
	{
		targetIdx = freeDescIndices.front(); // キューの先頭からインデックスを取得
		freeDescIndices.pop();              // キューから削除
	}
	else 
	{
		if (targetDescHeap.currentIdx >= MAX_DESC_COUNT)
		{
			OutputDebugStringA("Error: Descriptor Heap is full!\n");
		}
		targetIdx = targetDescHeap.currentIdx;
		targetDescHeap.currentIdx++; // 新規のみカウンタを進める
	}

	D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle = targetDescHeap.cpuHandle;
	cpuHandle.ptr += (UINT64)(targetIdx * IncSize);

	if (outGpuHandle)
	{
		*outGpuHandle = targetDescHeap.gpuHandle;
		outGpuHandle->ptr += (UINT64)(targetIdx * IncSize);
	}

	if (outIndex)
	{
		*outIndex = targetIdx;
	}
	return cpuHandle;
}

void GpuResourceManager::FreeDescriptor(uint32_t index)
{
	// 安全性のためのチェック（無効な値を弾く）
	if (index == -1 || index >= MAX_DESC_COUNT) return;

	// 空いたインデックスをキューにポイっと入れておく
	freeDescIndices.push(index);
}

uint32_t GpuResourceManager::CreateSRV(Microsoft::WRL::ComPtr<ID3D12Resource>& target, uint32_t numElements, uint32_t strideSize) {
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
	srvDesc.Format = DXGI_FORMAT_UNKNOWN;
	srvDesc.Buffer.FirstElement = 0;
	srvDesc.Buffer.NumElements = numElements;
	srvDesc.Buffer.StructureByteStride = strideSize;
	srvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;

	uint32_t outIndex = UINT32_MAX;
	D3D12_CPU_DESCRIPTOR_HANDLE srvHandleCPU =
		AllocateDescriptor(nullptr, &outIndex);


	if (outIndex == UINT32_MAX)
	{
		std::wstring name = GetDebugName(target.Get());
		Debugger::Log("SRV allocation failed : %ls\n", name.c_str());
		return UINT32_MAX;
	}

	m_dev->CreateShaderResourceView(
		target.Get(), &srvDesc, srvHandleCPU);

	return outIndex;
}

uint32_t GpuResourceManager::CreateUAV(Microsoft::WRL::ComPtr<ID3D12Resource>& target, uint32_t numElements, uint32_t strideSize) {
	D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
	uavDesc.Format = DXGI_FORMAT_UNKNOWN;
	uavDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
	uavDesc.Buffer.FirstElement = 0;
	uavDesc.Buffer.NumElements = numElements;
	uavDesc.Buffer.StructureByteStride = strideSize;
	uavDesc.Buffer.CounterOffsetInBytes = 0;
	uavDesc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_NONE;

	uint32_t outIndex = INT32_MAX;
	D3D12_CPU_DESCRIPTOR_HANDLE uavHandleCPU =
		AllocateDescriptor(nullptr, &outIndex);

	if (outIndex == UINT32_MAX)
	{
		std::wstring name = GetDebugName(target.Get());
		Debugger::Log("SRV allocation failed : %ls\n", name.c_str());
		return UINT32_MAX;
	}

	m_dev->CreateUnorderedAccessView(
		target.Get(), nullptr, &uavDesc, uavHandleCPU);

	return outIndex;
}

uint32_t GpuResourceManager::CreateCBV(Microsoft::WRL::ComPtr<ID3D12Resource>& target, uint32_t strideSize)
{
	uint32_t outIndex = UINT32_MAX;

	uint32_t alignedSize = (strideSize + 255) & ~255;

	D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
	cbvDesc.BufferLocation = target->GetGPUVirtualAddress();
	cbvDesc.SizeInBytes = static_cast<UINT>(alignedSize);

	D3D12_CPU_DESCRIPTOR_HANDLE cbvHandleCPU =
		AllocateDescriptor(nullptr, &outIndex);

	if (outIndex == UINT32_MAX)
	{
		std::wstring name = GetDebugName(target.Get());
		Debugger::Log("CBV allocation failed : %ls\n", name.c_str());
		return UINT32_MAX;
	}

	m_dev->CreateConstantBufferView(&cbvDesc, cbvHandleCPU);

	return outIndex;
}

void GpuResourceManager::Init(ID3D12Device* dev, Window& window) {
	m_dev = dev;
	SetUpDescHeap(window.GetHWND());
}