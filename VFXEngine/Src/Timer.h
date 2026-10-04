#pragma once
#include <windows.h>

class Timer {
public:
	void Init() {
		if (frequency.QuadPart == 0) {
			QueryPerformanceFrequency(&frequency);
		}
		QueryPerformanceCounter(&lastTime);
	}

	// 毎フレームの冒頭で1回だけ呼ぶ更新処理
	void Update() {
		LARGE_INTEGER currentTime;
		QueryPerformanceCounter(&currentTime);

		s_deltaTime = static_cast<float>(currentTime.QuadPart - lastTime.QuadPart) /
			static_cast<float>(frequency.QuadPart);

		lastTime = currentTime;
	}

	// 個別のデルタタイム取得（既存の関数）
	float GetDeltaTime() {
		LARGE_INTEGER currentTime;
		QueryPerformanceCounter(&currentTime);

		float deltaTime = static_cast<float>(currentTime.QuadPart - lastTime.QuadPart) /
			static_cast<float>(frequency.QuadPart);

		lastTime = currentTime;
		return deltaTime;
	}

	// ★ エンジン全体で共有するDeltaTimeを取得する static 関数
	static float GetGlobalDeltaTime() {
		return s_deltaTime;
	}

private:
	inline static LARGE_INTEGER frequency{};
	inline static float s_deltaTime = 0.0f; // ★ 全体で共有するDelta Time

	LARGE_INTEGER lastTime{};
};