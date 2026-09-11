#pragma once

namespace CommonRootParam {

	// -------------------------------------------------------------
	// 1. Root Parameter のスロット定義 (Root Signature のレイアウト)
	// -------------------------------------------------------------
	enum class RootParam {
		CommonIndices = 0, // 全 Compute 共通の Root Constants (1~2 DWORD)
		Count,
	};

	// -------------------------------------------------------------
	// 2. Root Constants (Slot 0) で直送りする共通インデックス
	// -------------------------------------------------------------
	enum class CommonIndex {
		SceneCBV = 0,           // シーン共通パラメータ
		PassParamCBV,           // ★各パス専用パラメータ構造体への CBV インデックス
		CurrentAssetCBV,
		CurrentInstCBV,
		AssetDynamicParamSRV,
		InstDynamicParamSRV,
		Count,
	};
}