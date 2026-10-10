#pragma once
#include <d3d12.h>
#include <wrl/client.h>
#include <functional>
#include <cstdint>

class Window;
class Dx12Wrapper;
class GpuResourceManager;

class PreviewUI {
	using RenderCallback = std::function<void()>;

private:
	Window* m_window = nullptr;
	Dx12Wrapper* m_dx12 = nullptr;
	GpuResourceManager* m_gpuResMgr = nullptr;

	uint32_t m_bufferWidth = 0;
	uint32_t m_bufferHeight = 0;

	bool m_needResize = false;
	uint32_t m_pendingWidth = 0;
	uint32_t m_pendingHeight = 0;

	struct ColorBuffer {
		Microsoft::WRL::ComPtr<ID3D12Resource> resource = nullptr;
		D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle{};
		D3D12_GPU_DESCRIPTOR_HANDLE srvHandle{};
	}m_colorBuffer{};

	struct DepthBuffer {
		Microsoft::WRL::ComPtr<ID3D12Resource> resource = nullptr;
		D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle{};
	}m_depthBuffer{};

	inline static RenderCallback m_renderCallback;

	HRESULT CreateColorBuffer(uint32_t width, uint32_t height);
	HRESULT CreateDepthBuffer(uint32_t width, uint32_t height);
	void ResizeBuffers(uint32_t width, uint32_t height);

public:
	struct InitDesc {
		Window& window;
		Dx12Wrapper& dx12;
		GpuResourceManager& gpuResMgr;
	};

	void Init(InitDesc& initDesc);
	void DrawInParentUI();
	void Render();

	static void RegisterRenderCallback(RenderCallback callback) {
		m_renderCallback = callback;
	}
};