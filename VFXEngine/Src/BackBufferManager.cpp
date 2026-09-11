#include "pch.h"
#include "BackBufferManager.h"
#include "Window.h"
#include "GpuResourceManager.h"
#include "Dx12Wrapper.h"

HRESULT BackBufferManager::CreateBackBuffers(Dx12Wrapper& dx12, GpuResourceManager& gpuResMgr)
{
    const auto& dev = dx12.GetDevice();

	for (int i = 0; i < BackBuffer::Count; ++i)
	{
		auto& target = m_backBuffers[i];

		// 1. スワップチェーンからバックバッファを取得
		if (FAILED(m_swapchain->GetBuffer(i, IID_PPV_ARGS(&target.resource)))) {
			OutputDebugStringA("Error: swapChain->GetBuffer failed!\n");
			return E_FAIL;
		}

		// 2. GPUリソースマネージャーから RTV 用の領域（CPUハンドル）を動的に確保
		target.rtvHandle = gpuResMgr.AllocateDescriptor(nullptr, nullptr, HeapType::Rtv);

		// 3. 確保したハンドル位置に RTV を作成
		dev->CreateRenderTargetView(target.resource.Get(), nullptr, target.rtvHandle);
	}

	return S_OK;
}

void BackBufferManager::PrepareForPresent(ID3D12GraphicsCommandList* cmdList)
{
    UINT bbIdx = GetCurrentBackBufferIndex();
	auto presentBarrier = CD3DX12_RESOURCE_BARRIER::Transition(
		m_backBuffers[bbIdx].resource.Get(),
		D3D12_RESOURCE_STATE_RENDER_TARGET,
		D3D12_RESOURCE_STATE_PRESENT);

	cmdList->ResourceBarrier(1, &presentBarrier);
}

void BackBufferManager::PrepareForRender(ID3D12GraphicsCommandList* cmdList)
{
    UINT bbIdx = GetCurrentBackBufferIndex();
    auto presentBarrier = CD3DX12_RESOURCE_BARRIER::Transition(
        m_backBuffers[bbIdx].resource.Get(),
        D3D12_RESOURCE_STATE_PRESENT,
        D3D12_RESOURCE_STATE_RENDER_TARGET);

    cmdList->ResourceBarrier(1, &presentBarrier);
}

HRESULT BackBufferManager::CreateSwapChain(HWND hwnd, Dx12Wrapper& dx12)
{
    const auto& cmdQueue = dx12.GetCmdQueue();
    const auto& dxgiFactory = dx12.GetDxgiFactory();

    DXGI_SWAP_CHAIN_DESC1 swapchainDesc = {};
    swapchainDesc.Width = 0;
    swapchainDesc.Height = 0;
    swapchainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    swapchainDesc.Stereo = FALSE;
    swapchainDesc.SampleDesc.Count = 1;
    swapchainDesc.SampleDesc.Quality = 0;
    swapchainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapchainDesc.BufferCount = static_cast<UINT>(BackBuffer::Count);
    swapchainDesc.Scaling = DXGI_SCALING_STRETCH;
    swapchainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    swapchainDesc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
    swapchainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

    // 1. 一時変数として SwapChain1 を用意
    Microsoft::WRL::ComPtr<IDXGISwapChain1> swapchain1;

    HRESULT hr = dxgiFactory->CreateSwapChainForHwnd(
        cmdQueue.Get(),
        hwnd,
        &swapchainDesc,
        nullptr,
        nullptr,
        swapchain1.GetAddressOf()
    );

    if (FAILED(hr)) {
        OutputDebugStringA("Swapchain creation failed\n");
        return hr;
    }

    // 2. IDXGISwapChain4 へ安全にインターフェース取得（As メソッドを使用）
    hr = swapchain1.As(&m_swapchain);
    if (FAILED(hr)) {
        OutputDebugStringA("Failed to cast to IDXGISwapChain4\n");
        return hr;
    }

    // 3. Alt+Enterでの予期せぬ画面切り替えを防止（アプリ側で制御する場合）
    dxgiFactory->MakeWindowAssociation(hwnd, DXGI_MWA_NO_ALT_ENTER);

    return hr;
}

void BackBufferManager::Init(Window& window, Dx12Wrapper& dx12, GpuResourceManager& gpuResMgr)
{
    CreateSwapChain(window.GetHWND(), dx12);
    CreateBackBuffers(dx12, gpuResMgr);
}