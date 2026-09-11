#pragma once

class Window;
class Dx12Wrapper;
class GpuResourceManager;

class SceneUI {
private:
	Window* m_window = nullptr;
	Dx12Wrapper* m_dx12 = nullptr;
	GpuResourceManager* m_gpuResMgr = nullptr;

	Microsoft::WRL::ComPtr<ID3D12Resource> m_buffer = nullptr;
	D3D12_CPU_DESCRIPTOR_HANDLE m_rtvHandle{};
	D3D12_GPU_DESCRIPTOR_HANDLE m_srvHandle{};
	
	HRESULT CreateResource();

public:
	struct InitDesc {
		Window& window;
		Dx12Wrapper& dx12;
		GpuResourceManager& gpuResMgr;
	};
	void Setup(InitDesc& initDesc);
	void ShowUI();
	void RenderScene();
};