#pragma once
#include "VFXInstance.h"

class VFXInstEditor {
private:
	VFXInstance* m_targetInst = nullptr; // 現在編集中のアセット（キャッシュ）
	bool m_isOpen = false;             // ウィンドウの開閉状態

public:
	void ShowUI();
	void SetTarget(VFXInstance* inst);
	void Open(VFXInstance& inst);
};