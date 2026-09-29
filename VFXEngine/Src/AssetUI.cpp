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

	// レイアウト用のサイズパラメータ（画像は枠サイズいっぱいに表示）
	const float frameSize = 64.0f;     // アイコン枠のサイズ
	const float iconSize = frameSize; // 画像のサイズ（枠内いっぱいに拡大）
	const float padding = 16.0f;     // アイコン同士の間隔
	const float cellSize = frameSize + padding;

	const float panelWidth = ImGui::GetContentRegionAvail().x;
	int columnCount = static_cast<int>(panelWidth / cellSize);
	if (columnCount < 1) columnCount = 1; // 最低1列確保

	if (ImGui::BeginTable("AssetGridTable", columnCount))
	{
		for (size_t i = 0; i < m_assets.size(); ++i)
		{
			const auto& asset = m_assets[i];

			ImGui::TableNextColumn(); // 次のセルに移動

			ImGui::PushID(static_cast<int>(asset.id));
			ImGui::PushID(static_cast<int>(i));

			ImGui::BeginGroup();

			ImVec2 startPos = ImGui::GetCursorPos();
			ImVec2 frameSizeVec(frameSize, frameSize);

			// 1. 選択・ヒット判定用の Selectable を配置
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

			// ドラッグ＆ドロップソースの設定
			if (ImGui::BeginDragDropSource())
			{
				// ★ asset 自体（AssetBaseInfo 構造体）をデータとして送信する
				ImGui::SetDragDropPayload("ASSET_ITEM", &asset, sizeof(AssetBaseInfo));

				// ドラッグ中にカーソルに追従するプレビュー表示
				if (asset.icon.imguiSrvHandle.ptr != 0)
				{
					ImTextureID userTextureID = (ImTextureID)asset.icon.imguiSrvHandle.ptr;
					ImGui::Image(userTextureID, ImVec2(32.0f, 32.0f));
					ImGui::SameLine();
				}
				ImGui::TextUnformatted(asset.name.c_str());

				ImGui::EndDragDropSource();
			}

			// 2. 枠線の描画（選択状態に応じて色を変更）
			ImVec2 pMin = ImGui::GetItemRectMin();
			ImVec2 pMax = ImGui::GetItemRectMax();
			ImU32 borderColor = isSelected
				? IM_COL32(255, 204, 0, 255)  // 選択時：黄色の枠線
				: IM_COL32(150, 150, 150, 255); // 通常時：グレーの枠線

			ImGui::GetWindowDrawList()->AddRect(pMin, pMax, borderColor, 4.0f);

			// 3. 画像を Selectable の上に被せて目一杯表示
			if (asset.icon.imguiSrvHandle.ptr != 0)
			{
				ImTextureID userTextureID = (ImTextureID)asset.icon.imguiSrvHandle.ptr;

				// CursorPos を直前の Selectable の開始位置に戻す
				float offsetX = (frameSize - iconSize) * 0.5f;
				float offsetY = (frameSize - iconSize) * 0.5f;
				ImGui::SetCursorPos(ImVec2(startPos.x + offsetX, startPos.y + offsetY));

				// 画像の描画（クリックやホバーの判定を邪魔しないよう配置）
				ImGui::Image(userTextureID, ImVec2(iconSize, iconSize));
			}

			// 4. アセット名のテキストを枠線の下に配置
			ImGui::SetCursorPos(ImVec2(startPos.x, startPos.y + frameSize + ImGui::GetStyle().ItemSpacing.y));

			ImGui::PushTextWrapPos(ImGui::GetCursorPos().x + frameSize);
			ImGui::TextUnformatted(asset.name.c_str());
			ImGui::PopTextWrapPos();

			ImGui::EndGroup(); // グループ終了

			ImGui::PopID(); // i の Pop
			ImGui::PopID(); // asset.id の Pop
		}

		ImGui::EndTable();
	}

	BeginWindowPopup();
	ExecuteDeletion();

	ImGui::End();
}

void AssetUI::BeginWindowPopup()
{
	// アセットの上ではなく、ウィンドウの背景を右クリックした場合に開く
	// 第2引数を true (デフォルト) にしておくと、既存のコンテキストメニューが開いている時は無視してくれます
	if (ImGui::BeginPopupContextWindow("AssetWindowContextMenu", ImGuiPopupFlags_MouseButtonRight | ImGuiPopupFlags_NoOpenOverItems))
	{
		ImGui::TextDisabled("Asset Management");
		ImGui::Separator();

		if (ImGui::BeginMenu("Create"))
		{
			if (ImGui::MenuItem("VFX"))
			{
				// 例: VFX作成コールバックの呼び出し、または作成モーダルを開くフラグを立てる
				auto it = m_creationCallbacks.find(AssetType::VFX);
				if (it != m_creationCallbacks.end() && it->second)
				{
					it->second(); // VFX作成処理の実行
				}
			}

			// 将来的に別のアセットタイプ（Material, Sound等）が増えた場合に追加可能
			/*
			if (ImGui::MenuItem("Sound"))
			{
				...
			}
			*/

			ImGui::EndMenu();
		}

		ImGui::EndPopup();
	}
}

void AssetUI::BeginAssetPopup(const AssetBaseInfo& asset)
{
	if (ImGui::BeginPopupContextItem("AssetContextMenu"))
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