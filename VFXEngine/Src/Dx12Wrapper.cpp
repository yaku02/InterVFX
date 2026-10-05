#include "pch.h"

#include "Dx12Wrapper.h"
#include "GpuResourceManager.h"
#include "BackBufferSetting.h"
#include "Window.h"

Dx12Wrapper::~Dx12Wrapper() {
	adapters.clear();
	tmpAdapter.Reset();
	_dxgiFactory.Reset();

	OutputDebugStringA("Dx12Wrapper Destroy\n");
}

void Dx12Wrapper::Init(Window& window)
{
	m_window = &window;

#ifdef _DEBUG
	// デバッグレイヤーをオン
	debugger.EnableDebugLayer();

#endif

#ifdef _DEBUG
	CreateDXGIFactory2(DXGI_CREATE_FACTORY_DEBUG, IID_PPV_ARGS(&_dxgiFactory));

#else
	CreateDXGIFactory1(IID_PPV_ARGS(&_dxgiFactory));

#endif

	InitAdapters();
	CreateDevice();
	CreateCmdAllocator();
	CreateCmdList();
	CreateCmdQueue();
	CreateDepth(m_window->GetWindowWidth(), m_window->GetWindowHeight());

	// ←ここに追加！
	ComPtr<ID3D12InfoQueue> infoQueue;
	if (SUCCEEDED(_dev->QueryInterface(IID_PPV_ARGS(&infoQueue))))
	{
		infoQueue->SetBreakOnSeverity(
			D3D12_MESSAGE_SEVERITY_CORRUPTION, TRUE);

		infoQueue->SetBreakOnSeverity(
			D3D12_MESSAGE_SEVERITY_ERROR, TRUE);
	}
}

void Dx12Wrapper::InitAdapters()
{
	// ここに特定の名前を持つアダプターオブジェクトが入る
	for (int i = 0;
		_dxgiFactory->EnumAdapters(i, &tmpAdapter) != DXGI_ERROR_NOT_FOUND;
		++i)
	{
		adapters.push_back(tmpAdapter.Get());
	}

	for (auto adpt : adapters)
	{
		DXGI_ADAPTER_DESC adesc = {};
		adpt->GetDesc(&adesc);

		std::wstring strDesc = adesc.Description;

		// 探したいアダプターの名前を確認
		if (strDesc.find(L"NVIDIA") != std::string::npos)
		{
			tmpAdapter = adpt;
			break;
		}
	}
}

void Dx12Wrapper::CreateDevice()
{
	// デバイス作成
	HRESULT hr = D3D12CreateDevice(tmpAdapter.Get(), D3D_FEATURE_LEVEL_12_1, IID_PPV_ARGS(&_dev));

	if (FAILED(hr)) {
		debugger.Log("D3D12CreateDevice failed.\n");
	}
	_dev->SetName(L"Engine_Device");
}

void Dx12Wrapper::CreateCmdAllocator()
{
	for (int i = 0; i < FrameSetting::Count; ++i) {
		// コマンドアロケーターの作成
		auto result = _dev->CreateCommandAllocator(
			D3D12_COMMAND_LIST_TYPE_DIRECT,
			IID_PPV_ARGS(&_cmdAllocators[i])
		);

		if (FAILED(result)) {
			OutputDebugStringA("Creation command allocator failed\n");
		}
	}
}

void Dx12Wrapper::CreateCmdList()
{
	auto result = _dev->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT,
		_cmdAllocators[0].Get(),
		nullptr,
		IID_PPV_ARGS(&_cmdList));

	if (FAILED(result)) {
		OutputDebugStringA("Creation cmdList failed\n");
	}
}

void Dx12Wrapper::CreateCmdQueue()
{
	// コマンドキュー作成

	D3D12_COMMAND_QUEUE_DESC cmdQueueDesc = {};

	// タイムアウトなし
	cmdQueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;

	// アダプターを1つしか使わない時は0でよい
	cmdQueueDesc.NodeMask = 0;

	// プライオリティは特に指定なし
	cmdQueueDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;

	// コマンドリストと合わせる
	cmdQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;

	// キュー作成
	auto result = _dev->CreateCommandQueue(&cmdQueueDesc, IID_PPV_ARGS(&_cmdQueue));
	if (result != S_OK) {
		OutputDebugStringA("cmdQueue is failed\n");
	}
	_cmdQueue->SetName(L"Engine_CmdQueue");
}

void Dx12Wrapper::CreateDepth(int window_width, int window_height)
{
	// 1. 深度バッファリソースの作成
	D3D12_RESOURCE_DESC depthResDesc = {};
	depthResDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	depthResDesc.Width = window_width;
	depthResDesc.Height = window_height;
	depthResDesc.DepthOrArraySize = 1;
	depthResDesc.MipLevels = 1;
	depthResDesc.Format = DXGI_FORMAT_D32_FLOAT; // 深度用フォーマット
	depthResDesc.SampleDesc.Count = 1;
	depthResDesc.SampleDesc.Quality = 0;
	depthResDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	depthResDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL; // 重要！

	// 深度のクリア値設定
	D3D12_CLEAR_VALUE depthClearValue = {};
	depthClearValue.Format = DXGI_FORMAT_D32_FLOAT;
	depthClearValue.DepthStencil.Depth = 1.0f; // 最大値(一番奥)でクリア
	depthClearValue.DepthStencil.Stencil = 0;

	D3D12_HEAP_PROPERTIES depthHeapProp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);

	_dev->CreateCommittedResource(
		&depthHeapProp,
		D3D12_HEAP_FLAG_NONE,
		&depthResDesc,
		D3D12_RESOURCE_STATE_DEPTH_WRITE, // 書き込み可能な状態で作成
		&depthClearValue,
		IID_PPV_ARGS(&depthBuffer));

	// 2. DSV用ディスクリプタヒープの作成
	D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc = {};
	dsvHeapDesc.NumDescriptors = 1;
	dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV; // 深度ステンシルビュー用
	dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

	_dev->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(&dsvHeap));

	// 3. ビュー(DSV)の作成
	D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
	dsvDesc.Format = DXGI_FORMAT_D32_FLOAT;
	dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
	dsvDesc.Flags = D3D12_DSV_FLAG_NONE;

	_dev->CreateDepthStencilView(depthBuffer.Get(), &dsvDesc, dsvHeap->GetCPUDescriptorHandleForHeapStart());
}

void Dx12Wrapper::ExecuteCommand() {
	// コマンド実行
	_cmdList->Close();
	ID3D12CommandList* lists[] = { _cmdList.Get() };
	_cmdQueue->ExecuteCommandLists(1, lists);
}

void Dx12Wrapper::ResetCommands(UINT backBufferIdx) {
	_cmdAllocators[backBufferIdx]->Reset();
	_cmdList->Reset(_cmdAllocators[backBufferIdx].Get(), nullptr);
}

void Dx12Wrapper::ExecuteInitCommands()
{
	// 1. 初期化用コマンドの記録を終了
	_cmdList->Close();

	// 2. コマンドキューで実行
	ID3D12CommandList* ppCmdLists[] = { _cmdList.Get() };
	_cmdQueue->ExecuteCommandLists(1, ppCmdLists);
}

void Dx12Wrapper::FlushCommandQueue()
{
	if (!_cmdQueue || !_dev) return;

	ComPtr<ID3D12Fence> tempFence;
	_dev->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&tempFence));

	HANDLE event = CreateEvent(nullptr, FALSE, FALSE, nullptr);
	_cmdQueue->Signal(tempFence.Get(), 1);

	if (tempFence->GetCompletedValue() < 1) {
		tempFence->SetEventOnCompletion(1, event);
		WaitForSingleObject(event, INFINITE);
	}

	CloseHandle(event);
}