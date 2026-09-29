#pragma once
#include "Debugger.h"
#include "FrameSetting.h"

class Window;
class Dx12Wrapper
{
	template <typename T>
	using ComPtr = Microsoft::WRL::ComPtr<T>;

private:
	Window* m_window = nullptr;
	Debugger debugger;

	ComPtr<IDXGIFactory6> _dxgiFactory = nullptr;
	std::vector <ComPtr<IDXGIAdapter>> adapters;
	ComPtr<IDXGIAdapter> tmpAdapter = nullptr;
	ComPtr<ID3D12Device> _dev = nullptr;
	ComPtr<ID3D12CommandAllocator> _cmdAllocators[FrameSetting::Count] = {};
	ComPtr<ID3D12GraphicsCommandList> _cmdList = nullptr;
	ComPtr<ID3D12CommandQueue> _cmdQueue = nullptr;
	ComPtr<ID3D12Resource> depthBuffer = nullptr;
	ComPtr<ID3D12DescriptorHeap> dsvHeap = nullptr;

	void InitAdapters();
	void CreateDevice();
	void CreateCmdAllocator();
	void CreateCmdList();
	void CreateCmdQueue();
	void CreateDepth(int window_width, int window_height);
	
public :

	~Dx12Wrapper();
	void Init(Window& window);
	void ExecuteInitCommands();
	void ResetCommands(UINT backBufferIdx);
	void ExecuteCommand();

	const ComPtr<ID3D12Device>& GetDevice() const { return _dev; }
	const ComPtr<ID3D12GraphicsCommandList>& GetCmdList() const { return _cmdList; }
	const ComPtr<ID3D12CommandQueue>& GetCmdQueue() const { return _cmdQueue; }
	const ComPtr<IDXGIFactory6>& GetDxgiFactory() const { return _dxgiFactory; }
};