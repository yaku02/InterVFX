#pragma once
#include "VFXAsset.h"

class VFXAssetEditor {
	using ChangeMaxParticleCallback = std::function<void(uint32_t id, uint32_t afterMaxParticles)>;

private:
	VFXAsset* m_targetAsset = nullptr; // 現在編集中のアセット（キャッシュ）
	bool m_isOpen = false;             // ウィンドウの開閉状態
	inline static ChangeMaxParticleCallback m_changeMaxParticleCallback{};

public:
	void ShowUI();
	void SetTarget(VFXAsset* asset);
	void Open(VFXAsset& asset);

	static void RegisterChangeMaxParticleCallback(ChangeMaxParticleCallback callback);
};