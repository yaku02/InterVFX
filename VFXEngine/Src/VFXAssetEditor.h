#pragma once
#include "VFXAsset.h"

class VFXAssetEditor {
private:
	VFXAsset* m_targetAsset = nullptr; // 現在編集中のアセット（キャッシュ）
	bool m_isOpen = false;             // ウィンドウの開閉状態

public:
	void ShowUI();
	void SetTarget(VFXAsset* asset);
	void Open(VFXAsset& asset);
};