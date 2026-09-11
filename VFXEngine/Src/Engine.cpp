#include "pch.h"
#include "Engine.h"
#include "BackBufferManager.h"
#include "Dx12Wrapper.h"

Engine::~Engine() {
    // 破棄前にGPUの処理完了を保証する
    if (m_dx12 && m_fence) {
        WaitForGpu();
    }
    if (event) CloseHandle(event);
}

void Engine::Init(BackBufferManager& backBufferMgr, Dx12Wrapper& dx12Wrapper)
{
    m_backBufferMgr = &backBufferMgr;
    m_dx12 = &dx12Wrapper;

    CreateFence();
}

HRESULT Engine::CreateFence()
{
    HRESULT result = m_dx12->GetDevice()->CreateFence(
        m_currentFenceVal, // 初期値 0
        D3D12_FENCE_FLAG_NONE,
        IID_PPV_ARGS(&m_fence));

    if (FAILED(result)) {
        OutputDebugStringA("Creation Fence failed\n");
        return result; // <--- 修正: HRESULTを返す
    }

    // イベントハンドルの作成
    event = CreateEvent(nullptr, FALSE, FALSE, nullptr);
    if (!event) {
        return HRESULT_FROM_WIN32(GetLastError());
    }

    return S_OK; // <--- 修正: 成功時の戻り値
}

void Engine::PrepareFrame() {
    // Present() によって既に更新された「次の描画用」インデックスを取得
    UINT bbIdx = m_backBufferMgr->GetCurrentBackBufferIndex();

    // 次のバックバッファがGPUで使用中であれば完了を待つ
    if (m_fence->GetCompletedValue() < m_fenceValues[bbIdx]) {
        m_fence->SetEventOnCompletion(m_fenceValues[bbIdx], event);
        WaitForSingleObject(event, INFINITE);
    }
}

void Engine::PresentAndSignal() {
    UINT bbIdx = m_backBufferMgr->GetCurrentBackBufferIndex();
    const auto& swapChain = m_backBufferMgr->GetSwapChain();
    const auto& cmdQueue = m_dx12->GetCmdQueue();

    // 画面表示（ここで内部の BackBufferIndex が更新される）
    swapChain->Present(1, 0);

    // 今描画し終えたバッファのフェンス値を更新して Signal
    m_fenceValues[bbIdx] = ++m_currentFenceVal;
    cmdQueue->Signal(m_fence.Get(), m_fenceValues[bbIdx]);
}

void Engine::WaitForGpu()
{
    if (!m_dx12 || !m_dx12->GetCmdQueue() || !m_fence) return;

    const auto& cmdQueue = m_dx12->GetCmdQueue();

    m_currentFenceVal++;

    cmdQueue->Signal(m_fence.Get(), m_currentFenceVal);

    if (m_fence->GetCompletedValue() < m_currentFenceVal)
    {
        m_fence->SetEventOnCompletion(m_currentFenceVal, event);
        WaitForSingleObject(event, INFINITE);
    }
}