#pragma once

class GpuResourceManager;

class HdrBuffer {
private:
	Microsoft::WRL::ComPtr<ID3D12Resource> m_buffer = nullptr;
	D3D12_CPU_DESCRIPTOR_HANDLE m_rtvHandle{};
	D3D12_GPU_DESCRIPTOR_HANDLE m_srvHandle{};
	uint32_t m_srvIndex = UINT32_MAX;

public:
	HRESULT CreateResource(ID3D12Device* dev, GpuResourceManager& gpuResMgr, int window_width, int window_height);
	void SetRenderTargetWithDepth(ID3D12GraphicsCommandList* cmdList, D3D12_CPU_DESCRIPTOR_HANDLE& dsvHandle);
};