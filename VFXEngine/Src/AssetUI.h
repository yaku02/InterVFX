#pragma once
#include "AssetStructs.h"

class AssetUI {
private:
	inline static std::vector<AssetInfo> m_assets;

public:
	void ShowUI();
	static void Add(const AssetInfo& info);
};