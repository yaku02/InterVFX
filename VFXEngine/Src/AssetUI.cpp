#include "pch.h"
#include "AssetUI.h"

void AssetUI::ShowUI()
{
	ImGui::Begin("Asset");

	ImGui::End();
}

void AssetUI::Add(const AssetInfo& info)
{
	m_assets.push_back(info);
}