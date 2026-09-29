#include "pch.h"
#include "InstUI.h"
#include "IDGenerator.h"
#include "Debugger.h"
#include "AssetStructs.h"

void InstUI::Add(const InstBaseInfo& inst)
{
	m_insts.push_back(inst);
}


void InstUI::RegisterDnDCreateCallback(InstType type, DnDCreateCallback callback)
{
	m_dndCreateCallbacks[type] = callback;
}

void InstUI::RegisterSelectCallback(InstType type, SelectCallback callback)
{
	m_selectCallbacks[type] = callback;
}

void InstUI::RegisterDeleteCallback(InstType type, DeleteCallback callBack)
{
	m_deleteCallbacks[type] = callBack;
}

void InstUI::ShowUI()
{
	ImGui::Begin("Instance");

	// --- リストヘッダー情報 ---
	ImGui::TextDisabled("Count: %zu", m_insts.size());
	ImGui::Separator();

	// --- インスタンス一覧表示 ---
	if (ImGui::BeginChild("InstanceList", ImVec2(0, 0), true))
	{
		uint32_t instToDelete = IDGenerator::INVALID_ID;
		InstType deleteType = InstType::Unknown;

		for (const auto& inst : m_insts)
		{
			// 選択状態の判定
			bool isSelected = (m_selectedInst.id == inst.id);

			// 一意のラベルを作成 (名前 ## ID)
			std::string label = inst.name + "##" + std::to_string(inst.id);

			// Selectable の描画
			if (ImGui::Selectable(label.c_str(), isSelected))
			{
				m_selectedInst = inst;

				// 選択コールバックの発火
				auto it = m_selectCallbacks.find(inst.type);
				if (it != m_selectCallbacks.end() && it->second)
				{
					it->second(inst.id);
				}
			}

			BeginInstPopup(inst);
		}
	}
	ImGui::EndChild();
	
	HandleDragDropTarget();
	ExecuteDeletion();

	ImGui::End();
}

void InstUI::BeginInstPopup(const InstBaseInfo& inst)
{
	if (ImGui::BeginPopupContextItem("InstContextMenu"))
	{
		m_selectedInst = inst;

		ImGui::TextDisabled("Name: %s", m_selectedInst.name.c_str());
		ImGui::Separator();

		if (ImGui::MenuItem("Delete"))
		{
			m_deleteTarget = m_selectedInst;
		}

		ImGui::EndPopup();
	}
}

void InstUI::ExecuteDeletion()
{
	if (m_deleteTarget.id == IDGenerator::INVALID_ID || m_deleteTarget.name.empty()) return;

	// 1. マネージャー側の削除コールバックを呼び出す
	auto it = m_deleteCallbacks.find(m_deleteTarget.type);
	if (it != m_deleteCallbacks.end() && it->second)
	{
		it->second(m_deleteTarget.id); // VFXManager::DeleteInstance(id) などが実行される
	}

	// 2. m_insts から削除対象の要素を除去する
	m_insts.erase(
		std::remove_if(m_insts.begin(), m_insts.end(),
			[this](const InstBaseInfo& inst) {
				return inst.id == m_deleteTarget.id;
			}),
		m_insts.end()
	);

	Debugger::Log("Delete Instance: %s", m_deleteTarget.name.c_str());

	// 3. 選択状態のクリア
	if (m_selectedInst.id == m_deleteTarget.id) {
		m_selectedInst = {};
	}
	m_deleteTarget = {};
}

void InstUI::HandleDragDropTarget()
{
	// ドラッグ＆ドロップターゲットの判定開始
	if (ImGui::BeginDragDropTarget())
	{
		// AssetUI の DragDropSource で設定したのと同じキー名 "ASSET_ITEM" で受け取る
		if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ASSET_ITEM"))
		{
			// 送られてきたデータサイズが AssetBaseInfo と一致するか安全チェック
			IM_ASSERT(payload->DataSize == sizeof(AssetBaseInfo));
			const AssetBaseInfo* droppedAsset = static_cast<const AssetBaseInfo*>(payload->Data);

			// 1. AssetBaseInfo に追加した instType を直接使用
			InstType instType = droppedAsset->instType;

			// 2. D&D用作成コールバックの検索と発火
			auto it = m_dndCreateCallbacks.find(instType);
			if (it != m_dndCreateCallbacks.end() && it->second)
			{
				// ドロップされた アセットの ID を渡して実行
				HRESULT hr = it->second(droppedAsset->id);
				if (SUCCEEDED(hr))
				{
					Debugger::Log("Created instance from Asset ID: %u (%s)\n", droppedAsset->id, droppedAsset->name.c_str());
				}
			}
			else
			{
				Debugger::Log("Warning: No DnD creation callback registered for InstType: %d\n", static_cast<int>(instType));
			}
		}

		ImGui::EndDragDropTarget();
	}
}