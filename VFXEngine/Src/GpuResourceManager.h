#pragma once
#include "GpuResourceStructs.h"

class Window;
class GpuResourceManager {
	template <typename T>
	using ComPtr = Microsoft::WRL::ComPtr<T>;

private:
	const UINT MAX_DESC_COUNT = 4096; // ディスクリプタの最大数
	UINT IncSize = 0;
	UINT RtvIncSize = 0;
	UINT DsvIncSize = 0;
	ComPtr<ID3D12Device> m_dev = nullptr;
	std::queue<uint32_t> freeDescIndices;

	DescHeapInfo m_visibleHeap;
	DescHeapInfo m_nonVisibleHeap;
	DescHeapInfo m_imguiHeap;
	DescHeapInfo m_rtvHeap;
	DescHeapInfo m_dsvHeap;

	void SetUpDescHeap(HWND hwnd);

public:
	GpuResourceManager() = default;
	~GpuResourceManager() = default;

	void Init(ID3D12Device* dev, Window& window);

	DescHeapInfo& GetDescHeap(HeapType type) {
		if (type == HeapType::Visible) return m_visibleHeap;
		else if (type == HeapType::NonVisible) return m_nonVisibleHeap;
		else if (type == HeapType::ImGui) return m_imguiHeap;
		else if (type == HeapType::Rtv) return m_rtvHeap;
		else if (type == HeapType::Dsv) return m_dsvHeap;
		return m_visibleHeap;
	}
	
	D3D12_CPU_DESCRIPTOR_HANDLE AllocateDescriptor(D3D12_GPU_DESCRIPTOR_HANDLE* outGpuHandle = nullptr, uint32_t* outIndex = nullptr, HeapType heapType = HeapType::Visible);

	D3D12_CPU_DESCRIPTOR_HANDLE GetCPUDescriptorHandle(uint32_t index, HeapType heapType);
	D3D12_GPU_DESCRIPTOR_HANDLE GetGPUDescriptorHandle(uint32_t index, HeapType heapType);

	inline D3D12_RESOURCE_STATES GetD3D12State(ResState state) {
		switch (state) {
		case ResState::Common:                 return D3D12_RESOURCE_STATE_COMMON;
		case ResState::VertexAndConstantBuffer:return D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
		case ResState::IndexBuffer:            return D3D12_RESOURCE_STATE_INDEX_BUFFER;
		case ResState::RenderTarget:           return D3D12_RESOURCE_STATE_RENDER_TARGET;
		case ResState::UnorderedAccess:        return D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
		case ResState::NonPixelShaderResource: return D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
		case ResState::PixelShaderResource:    return D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
		case ResState::IndirectArgument:       return D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT;
		case ResState::CopyDest:               return D3D12_RESOURCE_STATE_COPY_DEST;
		case ResState::CopySource:             return D3D12_RESOURCE_STATE_COPY_SOURCE;
		case ResState::Present:                return D3D12_RESOURCE_STATE_PRESENT;
		case ResState::DepthWrite:             return D3D12_RESOURCE_STATE_DEPTH_WRITE;
		case ResState::DepthRead:              return D3D12_RESOURCE_STATE_DEPTH_READ;
		}
		return D3D12_RESOURCE_STATE_COMMON;
	}

	void Transition(ID3D12GraphicsCommandList* cmdList, ID3D12Resource* resource, ResState before, ResState after);
	CD3DX12_RESOURCE_BARRIER Transition(ID3D12Resource* resource, ResState before, ResState after);

	template <typename T>
	HRESULT CreateConstantBuffer(Microsoft::WRL::ComPtr<ID3D12Resource>& targetBuffer, UINT64 bufferSize, T** mappedPtr, LPCWSTR name = L"uploadBuffer")
	{
		std::wstring wName = name;

		D3D12_HEAP_PROPERTIES heapProp =
			CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);

		D3D12_RESOURCE_DESC resDesc = 
			CD3DX12_RESOURCE_DESC::Buffer(bufferSize);

		auto result = m_dev->CreateCommittedResource(
			&heapProp,
			D3D12_HEAP_FLAG_NONE,
			&resDesc,
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(targetBuffer.ReleaseAndGetAddressOf()));

		if (FAILED(result))
		{
			std::wstring errorMsg = L"[Error] Creation " + wName + L" failed\n";
			OutputDebugStringW(errorMsg.c_str());
			return result;
		}

		targetBuffer->SetName(name);

		if (mappedPtr)
		{
			result = targetBuffer->Map(0, nullptr, reinterpret_cast<void**>(mappedPtr));
			if (FAILED(result))
			{
				std::wstring errorMsg = L"[Error] Map " + wName + L" failed\n";
				OutputDebugStringW(errorMsg.c_str());
				return result;
			}
		}

		return S_OK;
	}

	uint32_t CreateSRV(Microsoft::WRL::ComPtr<ID3D12Resource>& target, uint32_t numElements, uint32_t strideSize);
	uint32_t CreateUAV(Microsoft::WRL::ComPtr<ID3D12Resource>& target, uint32_t numElements, uint32_t strideSize);
	uint32_t CreateCBV(Microsoft::WRL::ComPtr<ID3D12Resource>& target, uint32_t strideSize);
	uint32_t CreateAppendUAV(Microsoft::WRL::ComPtr<ID3D12Resource>& target, Microsoft::WRL::ComPtr<ID3D12Resource>& counterResource, uint32_t numElements, uint32_t strideSize);

	// ディスクリプタインデックスの解放をする
	void FreeDescriptor(uint32_t index);
	std::wstring GetDebugName(ID3D12Object* object)
	{
		if (!object) return L"(null)";

		UINT size = 0;
		if (FAILED(object->GetPrivateData(WKPDID_D3DDebugObjectNameW, &size, nullptr)))
		{
			return L"(No Name)";
		}

		std::wstring name(size / sizeof(wchar_t), L'\0');

		if (FAILED(object->GetPrivateData(WKPDID_D3DDebugObjectNameW, &size, name.data())))
		{
			return L"(No Name)";
		}

		if (!name.empty() && name.back() == L'\0')
			name.pop_back();

		return name;
	}
};