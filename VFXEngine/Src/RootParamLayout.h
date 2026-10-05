#pragma once

namespace RootParam {

	// -------------------------------------------------------------
	// 1. Root Parameter のスロット定義 (Root Signature のレイアウト)
	// -------------------------------------------------------------
	enum class Slot : UINT{
		CBVIndices = 0, // 全 Compute 共通の Root Constants (1~2 DWORD)
		Count,
	};

	// -------------------------------------------------------------
	// 2. Root Constants (Slot 0) で直送りする共通インデックス
	// -------------------------------------------------------------
	enum class CBVIndices : UINT{
		GlobalCBVIndex = 0,
		PassCBVIndex,
		AssetCBVIndex,
		InstCBVIndex,
		Count,
	};
}