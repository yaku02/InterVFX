#include "pch.h"
#include "Window.h"
#include "Dx12Wrapper.h"
#include "Engine.h"
#include "BackBufferManager.h"
#include "GpuResourceManager.h"
#include "UIManager.h"
#include "Debugger.h"
#include "TextureManager.h"
#include "RootSignatureManager.h"
#include "VFXManager.h"
#include "Timer.h"

#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(lib, "dxcompiler.lib")
#pragma comment(lib, "dxguid.lib") 
#pragma comment(lib, "dwmapi.lib")

int main()
{
	{
		Window window;
		window.Setup();

		Dx12Wrapper dx12Wrapper;
		dx12Wrapper.Init(window);

		GpuResourceManager gpuResMgr;
		gpuResMgr.Init(dx12Wrapper.GetDevice().Get(), window);

		BackBufferManager backBufferMgr;
		backBufferMgr.Init(window, dx12Wrapper, gpuResMgr);

		Engine engine;
		engine.Init(backBufferMgr, dx12Wrapper);

		TextureManager texMgr;
		{
			TextureManager::InitDesc initDesc{
				.dx12 = dx12Wrapper,
				.gpuResMgr = gpuResMgr
			};
			texMgr.Init(initDesc);
		}

		RootSignatureManager rootSigMgr;
		{
			rootSigMgr.Init(dx12Wrapper.GetDevice().Get());
		}

		VFXManager vfxMgr;
		{
			VFXManager::InitDesc initDesc{
				.window = window,
				.dx12 = dx12Wrapper,
				.gpuResMgr = gpuResMgr,
				.texMgr = texMgr
			};
			vfxMgr.Init(initDesc);
		}

		UIManager uiMgr;
		{
			UIManager::InitDesc initDesc{
				.window = window,
				.dx12 = dx12Wrapper,
				.gpuResMgr = gpuResMgr,
				.backBufferMgr = backBufferMgr,
				.vfxAssetEditor = vfxMgr.GetAssetEditor(),
				.vfxInstEditor = vfxMgr.GetInstEditor(),
			};
			uiMgr.Init(initDesc);
		}

		Timer mainTimer;
		mainTimer.Init();
		
		dx12Wrapper.ExecuteInitCommands();
		engine.WaitForGpu();
		Debugger::Log("Initialize succeeded\n");
		
		MSG msg = {};
		bool isRunning = true;
		while (isRunning)
		{
			using namespace DirectX;

			while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
			{
				if (msg.message == WM_QUIT) {
					isRunning = false;
					break; // 終了
				}

				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
			if (!isRunning) break;

			mainTimer.Update();

			engine.PrepareFrame();
			dx12Wrapper.ResetCommands(backBufferMgr.GetCurrentBackBufferIndex());
			backBufferMgr.PrepareForRender(dx12Wrapper.GetCmdList().Get());

			// --- ここから追加 ---
			UINT bbIdx = backBufferMgr.GetCurrentBackBufferIndex();

			// レンダーターゲットビュー（RTV）のハンドルを取得
			D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = backBufferMgr.GetCurrentBackBufferRtvHandle();

			// クリアカラーの指定 (R, G, B, A)
			float clearColor[] = { 0.1f, 0.2f, 0.4f, 1.0f }; // 青っぽい色

			// 画面のクリア命令を発行
			auto* cmdList = dx12Wrapper.GetCmdList().Get();
			cmdList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);
			// --- ここまで追加 ---

			uiMgr.ExecuteRendering();

			backBufferMgr.PrepareForPresent(dx12Wrapper.GetCmdList().Get());
			dx12Wrapper.ExecuteCommand();
			engine.PresentAndSignal();

		}

		engine.WaitForGpu();
		uiMgr.ShutDown();
		window.ShutDown();
		vfxMgr.ShutDown();
	}

#ifdef _DEBUG
	OutputDebugStringA("=== LIVE OBJECT REPORT ===\n");

	Microsoft::WRL::ComPtr<IDXGIDebug1> dxgiDebug;
	if (SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&dxgiDebug))))
	{
		dxgiDebug->ReportLiveObjects(
			DXGI_DEBUG_ALL,
			DXGI_DEBUG_RLO_DETAIL
		);
	}
	OutputDebugStringA("=== END REPORT ===\n");
#endif

	return 0;
}