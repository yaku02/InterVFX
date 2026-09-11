#pragma once
#include "pch.h"
#include "BackBufferSetting.h"

class Window;
class GpuResourceManager;
class Dx12Wrapper;

class BackBufferManager {
	template <typename T>
	using ComPtr = Microsoft::WRL::ComPtr<T>;

private:
	BackBuffer m_backBuffers[BackBuffer::Count];
	ComPtr<IDXGISwapChain4> m_swapchain = nullptr;

	HRESULT CreateBackBuffers(Dx12Wrapper& dx12, GpuResourceManager& gpuResMgr);
	HRESULT CreateSwapChain(HWND hwnd, Dx12Wrapper& dx12);

public:

	void Init(Window& window, Dx12Wrapper& dx12, GpuResourceManager& gpuResMgr);
	void PrepareForPresent(ID3D12GraphicsCommandList* cmdList);
	void PrepareForRender(ID3D12GraphicsCommandList* cmdList);

	UINT GetCurrentBackBufferIndex() const { return m_swapchain->GetCurrentBackBufferIndex(); }
	ComPtr<IDXGISwapChain4> GetSwapChain() const { return m_swapchain; }

	const D3D12_CPU_DESCRIPTOR_HANDLE& GetCurrentBackBufferRtvHandle() const {
		return m_backBuffers[GetCurrentBackBufferIndex()].rtvHandle;
	}
};