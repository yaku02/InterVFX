#include "pch.h"
#include "VFXInstEditor.h"
#include "IDGenerator.h"
#include "UIManager.h"

void VFXInstEditor::SetTarget(VFXInstance* inst)
{
	m_targetInst = inst;
	m_isOpen = (m_targetInst != nullptr);
}

void VFXInstEditor::ShowUI()
{
	// ウィンドウが閉じている、またはターゲットがなければ何も描画しない
	if (!m_isOpen || !m_targetInst) return;

	// キャッシュしたポインタを参照して描画を実行
	Open(*m_targetInst);
}

void VFXInstEditor::Open(VFXInstance& inst)
{
	if (!m_targetInst || m_targetInst->info.base.id == IDGenerator::INVALID_ID)
	{
		ImGui::TextDisabled("No VFX Instance Selected.");
		return;
	}

	bool isChanged = false;

	// テーブル作成用のヘルパーラムダ
	auto BeginPropertyTable = [](const char* id) {
		return ImGui::BeginTable(id, 2, ImGuiTableFlags_SizingStretchProp | ImGuiTableFlags_PadOuterX);
		};

	// --- 1. Basic Info (基本情報) ---
	if (ImGui::CollapsingHeader("Basic Info", ImGuiTreeNodeFlags_DefaultOpen))
	{
		if (BeginPropertyTable("InstBasicInfoTable"))
		{
			// Instance Name
			ImGui::TableNextColumn();
			ImGui::TextUnformatted("Instance Name");
			ImGui::TableNextColumn();
			ImGui::SetNextItemWidth(-FLT_MIN);
			char nameBuffer[256];
			strncpy_s(nameBuffer, inst.info.base.name.c_str(), sizeof(nameBuffer));
			if (ImGui::InputText("##InstName", nameBuffer, sizeof(nameBuffer)))
			{
				inst.info.base.name = nameBuffer;
			}

			// Instance ID
			ImGui::TableNextColumn();
			ImGui::TextUnformatted("Instance ID");
			ImGui::TableNextColumn();
			ImGui::Text("%u", inst.info.base.id);

			// Source Asset Name / ID
			ImGui::TableNextColumn();
			ImGui::TextUnformatted("Source Asset");
			ImGui::TableNextColumn();
			ImGui::Text("%s (ID: %u)", inst.info.assetName.c_str(), inst.info.assetID);

			// Active & Visible Flags
			ImGui::TableNextColumn();
			ImGui::TextUnformatted("State");
			ImGui::TableNextColumn();
			ImGui::Checkbox("Active", &inst.cpuParam.isActive);
			ImGui::SameLine();
			ImGui::Checkbox("Visible", &inst.cpuParam.isVisible);

			ImGui::EndTable();
		}
	}

	ImGui::Spacing();

	// --- 2. Transform (トランスフォーム) ---
	if (ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen))
	{
		if (BeginPropertyTable("InstTransformTable"))
		{
			// Position
			ImGui::TableNextColumn();
			ImGui::TextUnformatted("Position");
			ImGui::TableNextColumn();
			ImGui::SetNextItemWidth(-FLT_MIN);
			if (ImGui::DragFloat3("##Position", &inst.cpuParam.position.x, 0.1f, -10000.0f, 10000.0f, "%.2f"))
			{
				isChanged = true;
			}

			// Rotation (度数法で操作できるように調整)
			ImGui::TableNextColumn();
			ImGui::TextUnformatted("Rotation");
			ImGui::TableNextColumn();
			ImGui::SetNextItemWidth(-FLT_MIN);
			if (ImGui::DragFloat3("##Rotation", &inst.cpuParam.rotation.x, 0.5f, -360.0f, 360.0f, "%.1f deg"))
			{
				isChanged = true;
			}

			// Scale
			ImGui::TableNextColumn();
			ImGui::TextUnformatted("Scale");
			ImGui::TableNextColumn();
			ImGui::SetNextItemWidth(-FLT_MIN);
			if (ImGui::DragFloat3("##Scale", &inst.cpuParam.scale.x, 0.01f, 0.001f, 1000.0f, "%.3f"))
			{
				isChanged = true;
			}

			ImGui::EndTable();
		}
	}

	ImGui::Spacing();

	// --- 3. Behavior (挙動・移動パラメーター) ---
	if (ImGui::CollapsingHeader("Behavior", ImGuiTreeNodeFlags_DefaultOpen))
	{
		if (BeginPropertyTable("InstBehaviorTable"))
		{
			// Direction
			ImGui::TableNextColumn();
			ImGui::TextUnformatted("Direction");
			ImGui::TableNextColumn();
			ImGui::SetNextItemWidth(-FLT_MIN);
			if (ImGui::DragFloat3("##Direction", &inst.gpuParam.behavior.direction.x, 0.01f, -1.0f, 1.0f, "%.2f"))
			{
				isChanged = true;
			}

			// Speed
			ImGui::TableNextColumn();
			ImGui::TextUnformatted("Speed");
			ImGui::TableNextColumn();
			ImGui::SetNextItemWidth(-FLT_MIN);
			if (ImGui::DragFloat("##Speed", &inst.gpuParam.behavior.speed, 0.1f, 0.0f, 1000.0f, "%.2f"))
			{
				isChanged = true;
			}

			ImGui::EndTable();
		}
	}

	ImGui::Spacing();

	// パラメータが変更された場合、World行列を再計算して定数バッファを更新
	if (isChanged)
	{
		// 1. World 行列の更新 (Scale * Rotation * Translation)
		using namespace DirectX;
		XMMATRIX scaleMat = XMMatrixScaling(inst.cpuParam.scale.x, inst.cpuParam.scale.y, inst.cpuParam.scale.z);
		XMMATRIX rotMat = XMMatrixRotationRollPitchYaw(XMConvertToRadians(inst.cpuParam.rotation.x),
			XMConvertToRadians(inst.cpuParam.rotation.y),
			XMConvertToRadians(inst.cpuParam.rotation.z));
		XMMATRIX transMat = XMMatrixTranslation(inst.cpuParam.position.x, inst.cpuParam.position.y, inst.cpuParam.position.z);

		inst.cpuParam.world = scaleMat * rotMat * transMat;

		// 2. GPU ConstantBuffer への転送
		inst.UploadConstBuff();
	}
}