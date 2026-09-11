#include "pch.h"
#include "Window.h"

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
LRESULT Window::WindowProcedure(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
	// ImGuiのコンテキストが存在しない場合は OS のデフォルト処理へ流す
	if (ImGui::GetCurrentContext() == nullptr) {
		return DefWindowProc(hwnd, msg, wparam, lparam); // ★ false から修正
	}

	// 1. ImGui にイベントを渡す（ImGui がキャプチャした場合は true が返る）
	if (ImGui_ImplWin32_WndProcHandler(hwnd, msg, wparam, lparam)) {
		return true;
	}

	// 2. Windows標準のメッセージ処理
	switch (msg)
	{
	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;

	case WM_SIZE:
		return 0;

	case WM_KEYDOWN:
		if (wparam == VK_ESCAPE) {
			PostMessage(hwnd, WM_CLOSE, 0, 0);
		}
		return 0;
	}

	return DefWindowProc(hwnd, msg, wparam, lparam);
}

void Window::Setup()
{
	SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);

	m_wndc.cbSize = sizeof(WNDCLASSEX);
	m_wndc.lpfnWndProc = Window::WindowProcedure; // コールバック関数の指定
	m_wndc.lpszClassName = _T("DX12Sample");       // アプリケーションクラス名
	m_wndc.hInstance = GetModuleHandle(nullptr);   //ハンドルの取得

	RegisterClassEx(&m_wndc); // アプリケーションクラス （ウィンドウクラスの指定をOSに伝える）

	RECT wrc = { 0, 0, m_window_width, m_window_height }; // ウィンドウサイズを決める

	// 関数を使ってウィンドウサイズを補正する
	AdjustWindowRect(&wrc, WS_OVERLAPPEDWINDOW, false);

	// ウィンドウオブジェクトの作成
	m_hwnd = CreateWindow(
		m_wndc.lpszClassName,
		_T("InterVFX"),              // タイトルバーの文字
		WS_OVERLAPPEDWINDOW,           // タイトルバーと境界線があるウィンドウ
		CW_USEDEFAULT,                 // 表示x座標はOSにお任せ
		CW_USEDEFAULT,                 // 表示y座標はOSにお任せ
		wrc.right - wrc.left,          // ウィンドウ幅
		wrc.bottom - wrc.top,          // ウィンドウ高
		nullptr,                       // 親ウィンドウハンドル
		nullptr,                       // メニューハンドル
		m_wndc.hInstance,                   // 呼び出しアプリケーdションハンドル
		nullptr                        // 追加パラメーター
	);

	// ウィンドウ表示
	ShowWindow(m_hwnd, SW_SHOW);

	BOOL useDarkMode = TRUE;
	DwmSetWindowAttribute(
		m_hwnd,
		DWMWA_USE_IMMERSIVE_DARK_MODE,
		&useDarkMode,
		sizeof(useDarkMode)
	);
}

void Window::ShutDown()
{
	UnregisterClass(m_wndc.lpszClassName, m_wndc.hInstance);
}