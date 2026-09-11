#pragma once

class Window {

private:

	int m_window_width = 1920;
	int m_window_height = 1080;

	HWND m_hwnd;
	WNDCLASSEX m_wndc = {};

public:
	static LRESULT CALLBACK WindowProcedure(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);
	void Setup();
	void ShutDown();

	const HWND GetHWND() const { return m_hwnd; }
	int GetWindowWidth() { return m_window_width; }
	int GetWindowHeight() { return m_window_height; }
};