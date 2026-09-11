#pragma once
#include "FrameSetting.h"

class BackBufferManager;
class Dx12Wrapper;

class Engine {
	template <typename T>
	using ComPtr = Microsoft::WRL::ComPtr<T>;
private:
	BackBufferManager* m_backBufferMgr = nullptr;
	Dx12Wrapper* m_dx12 = nullptr;

	ComPtr<ID3D12Fence> m_fence = nullptr;
	UINT64 m_fenceValues[FrameSetting::Count] = { 0, 0 }; // 各アロケーターがいつ使い終わるかの記録用
	UINT64 m_currentFenceVal = 0;     // 全体でカウントアップしていく値

	HANDLE event;

public:
	~Engine();
	void Init(BackBufferManager& backBufferMgr, Dx12Wrapper& dx12Wrapper);
	HRESULT CreateFence();
	void PresentAndSignal();
	void PrepareFrame();
	void WaitForGpu();
};