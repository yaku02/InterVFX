#include "pch.h"
#include "AssetUI.h"
#include "IDGenerator.h"
#include "Debugger.h"

void AssetUI::Init(const InitDesc& desc)
{
}

void AssetUI::Add(const AssetBaseInfo& info)
{
	m_assets.push_back(info);
}

void AssetUI::RegisterCreationCallback(AssetType type, CreationCallback callback)
{
    Debugger::Log("RegisterCreationCallback");
	m_creationCallbacks[type] = callback;
}

void AssetUI::RegisterDeleteCallback(AssetType type, DeleteCallback callBack)
{
	m_deleteCallbacks[type] = callBack;
}

void AssetUI::RegisterSelectCallback(AssetType type, SelectCallback callback)
{
	m_selectCallbacks[type] = callback;
}

void AssetUI::ShowUI()
{
    ImGui::Begin("Asset");

    // ★ ウィンドウの右クリックメニューは Table 描画より前で行う
    BeginWindowPopup();

    const float frameSize = 64.0f;
    const float iconSize = frameSize;
    const float padding = 16.0f;
    const float cellSize = frameSize + padding;

    const float panelWidth = ImGui::GetContentRegionAvail().x;
    int columnCount = static_cast<int>(panelWidth / cellSize);
    if (columnCount < 1) columnCount = 1;

    if (ImGui::BeginTable("AssetGridTable", columnCount))
    {
        for (size_t i = 0; i < m_assets.size(); ++i)
        {
            const auto& asset = m_assets[i];

            ImGui::TableNextColumn();

            ImGui::PushID(static_cast<int>(asset.id));
            ImGui::PushID(static_cast<int>(i));

            ImGui::BeginGroup();

            ImVec2 startPos = ImGui::GetCursorPos();
            ImVec2 frameSizeVec(frameSize, frameSize);

            const bool isSelected = (m_selectedAsset.id == asset.id);
            if (ImGui::Selectable("##Selectable", isSelected, 0, frameSizeVec))
            {
                m_selectedAsset = asset;
                auto it = m_selectCallbacks.find(asset.type);
                if (it != m_selectCallbacks.end() && it->second)
                {
                    it->second(asset.id);
                }
            }

            BeginAssetPopup(asset);

            if (ImGui::BeginDragDropSource())
            {
                ImGui::SetDragDropPayload("ASSET_ITEM", &asset, sizeof(AssetBaseInfo));

                if (asset.icon.imguiSrvHandle.ptr != 0)
                {
                    ImTextureID userTextureID = (ImTextureID)asset.icon.imguiSrvHandle.ptr;
                    ImGui::Image(userTextureID, ImVec2(32.0f, 32.0f));
                    ImGui::SameLine();
                }
                ImGui::TextUnformatted(asset.name.c_str());

                ImGui::EndDragDropSource();
            }

            ImVec2 pMin = ImGui::GetItemRectMin();
            ImVec2 pMax = ImGui::GetItemRectMax();
            ImU32 borderColor = isSelected
                ? IM_COL32(255, 204, 0, 255)
                : IM_COL32(150, 150, 150, 255);

            ImGui::GetWindowDrawList()->AddRect(pMin, pMax, borderColor, 4.0f);

            if (asset.icon.imguiSrvHandle.ptr != 0)
            {
                ImTextureID userTextureID = (ImTextureID)asset.icon.imguiSrvHandle.ptr;
                float offsetX = (frameSize - iconSize) * 0.5f;
                float offsetY = (frameSize - iconSize) * 0.5f;
                ImGui::SetCursorPos(ImVec2(startPos.x + offsetX, startPos.y + offsetY));
                ImGui::Image(userTextureID, ImVec2(iconSize, iconSize));
            }

            ImGui::SetCursorPos(ImVec2(startPos.x, startPos.y + frameSize + ImGui::GetStyle().ItemSpacing.y));
            ImGui::PushTextWrapPos(ImGui::GetCursorPos().x + frameSize);
            ImGui::TextUnformatted(asset.name.c_str());
            ImGui::PopTextWrapPos();

            ImGui::EndGroup();

            ImGui::PopID();
            ImGui::PopID();
        }

        ImGui::EndTable();
    }

    ExecuteDeletion();

    ImGui::End();
}

void AssetUI::BeginWindowPopup()
{
	// ★ ImGuiPopupFlags_NoOpenOverItems を外し、BeginPopupContextWindow だけにする
	if (ImGui::BeginPopupContextWindow("AssetWindowContextMenu", ImGuiPopupFlags_MouseButtonRight))
	{
		ImGui::TextDisabled("Asset Management");
		ImGui::Separator();

		if (ImGui::BeginMenu("Create"))
		{
            if (ImGui::MenuItem("VFX"))
            {
                Debugger::Log("Menu Click");

                auto it = m_creationCallbacks.find(AssetType::VFX);

                Debugger::Log("callback count = %d", (int)m_creationCallbacks.size());

                if (it == m_creationCallbacks.end())
                {
                    Debugger::Log("NOT FOUND");
                }
                else
                {
                    Debugger::Log("FOUND");
                    it->second();
                }
            }

			ImGui::EndMenu();
		}

		ImGui::EndPopup();
	}
}

void AssetUI::BeginAssetPopup(const AssetBaseInfo& asset)
{
	std::string popupId = "AssetContextMenu##" + std::to_string(asset.id);

	if (ImGui::BeginPopupContextItem(popupId.c_str()))
	{
		m_selectedAsset = asset;

		ImGui::TextDisabled("Name: %s", m_selectedAsset.name.c_str());
		ImGui::Separator();

		if (ImGui::MenuItem("Delete"))
		{
			m_deleteTarget = m_selectedAsset;
		}

		ImGui::EndPopup();
	}
}

void AssetUI::ExecuteDeletion()
{
	if (m_deleteTarget.id == IDGenerator::INVALID_ID || m_deleteTarget.name.empty()) return;

	// 1. マネージャー側の削除コールバックを呼び出す
	auto it = m_deleteCallbacks.find(m_deleteTarget.type);
	if (it != m_deleteCallbacks.end() && it->second)
	{
		it->second(m_deleteTarget.id); // VFXManager::DeleteAsset(id) などが実行される
	}

	// 2. m_assets から削除対象の要素を除去する
	m_assets.erase(
		std::remove_if(m_assets.begin(), m_assets.end(),
			[this](const AssetBaseInfo& asset) {
				return asset.id == m_deleteTarget.id;
			}),
		m_assets.end()
	);

	Debugger::Log("Delete Asset: %s", m_deleteTarget.name.c_str());

	// 3. 選択状態のクリア
	if (m_selectedAsset.id == m_deleteTarget.id) {
		m_selectedAsset = {};
	}
	m_deleteTarget = {};
}

void AssetUI::ShutDown()
{
    m_assets.clear();
    m_selectedAsset = {};
    m_deleteTarget = {};
}