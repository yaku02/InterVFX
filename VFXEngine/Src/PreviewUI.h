#pragma once
#include "InstanceStructs.h"

class Window;
class Dx12Wrapper;
class GpuResourceManager;

class PreviewUI {
	using RenderingCallback = std::function<void(uint32_t id)>;
private:
	Window* m_window = nullptr;
	Dx12Wrapper* m_dx12 = nullptr;
	GpuResourceManager* m_gpuResMgr = nullptr;

	Microsoft::WRL::ComPtr<ID3D12Resource> m_buffer = nullptr;
	D3D12_CPU_DESCRIPTOR_HANDLE m_rtvHandle{};
	D3D12_GPU_DESCRIPTOR_HANDLE m_srvHandle{};

	static inline std::unordered_map<InstType, RenderingCallback> m_renderingCallbacks{};

	HRESULT CreateResource();

public:
	struct InitDesc {
		Window* window;
		Dx12Wrapper* dx12;
		GpuResourceManager* gpuResMgr;
	};
	void Init(const InitDesc& desc);
	void Render();
	void ShowUI();

	static void RegisterRenderingCallback(InstType, RenderingCallback callback);
};