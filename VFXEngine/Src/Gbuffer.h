#pragma once

class GpuResourceManager;
class Gbuffer {
private:
	const uint32_t m_bufferCount = 3;
	Microsoft::WRL::ComPtr<ID3D12Resource> m_buffers[3] = { nullptr, nullptr };
	D3D12_CPU_DESCRIPTOR_HANDLE m_rtvHandles[3]{};
	D3D12_GPU_DESCRIPTOR_HANDLE m_srvHandles[3]{};

public:
	HRESULT CreateResource(ID3D12Device* dev, GpuResourceManager& gpuResMgr, int window_width, int window_height);
	void SetRenderTargetWithDepth(ID3D12GraphicsCommandList* cmdList, D3D12_CPU_DESCRIPTOR_HANDLE& dsvHandle);

};