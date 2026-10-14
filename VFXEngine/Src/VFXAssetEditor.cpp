#include "pch.h"
#include "VFXAssetEditor.h"
#include "VFXAsset.h"
#include "IDGenerator.h"
#include "UIManager.h"
#include "Debugger.h"

void VFXAssetEditor::RegisterChangeMaxParticleCallback(ChangeMaxParticleCallback callback) {
	m_changeMaxParticleCallback = callback;
}

void VFXAssetEditor::SetTarget(VFXAsset* asset)
{
	m_targetAsset = asset;
	m_isOpen = (m_targetAsset != nullptr);
}

void VFXAssetEditor::ShowUI()
{
	// ウィンドウが閉じている、またはターゲットがなければ何も描画しない
	if (!m_isOpen || !m_targetAsset) return;

	// キャッシュしたポインタを参照して描画を実行
	Open(*m_targetAsset);
}

void VFXAssetEditor::Open(VFXAsset& asset)
{
	if (!m_targetAsset || m_targetAsset->info.base.id == IDGenerator::INVALID_ID)
	{
		ImGui::TextDisabled("No VFX Asset Selected.");
		return;
	}

	ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)ImColor::HSV(0.55f, 0.6f, 0.7f));
	ImGui::PushStyleColor(ImGuiCol_ButtonHovered, (ImVec4)ImColor::HSV(0.55f, 0.7f, 0.8f));
	ImGui::PushStyleColor(ImGuiCol_ButtonActive, (ImVec4)ImColor::HSV(0.55f, 0.8f, 0.9f));

	ImGui::Button(reinterpret_cast<const char*>(u8"ノードエディタに切り替え"));

	if (ImGui::Button("ノードエディタに切り替え", ImVec2(-FLT_MIN, 0)))
	{
		Debugger::Log("Switch to Node Editor clicked for Asset ID: %u\n", asset.info.base.id);
		// TODO: ここに実際のノードエディタへの切り替え処理を記述する
	}

	ImGui::PopStyleColor(3); // プッシュした色を戻す

	bool isChanged = false;

	// 2列用（標準: ラベル + 入力）
	auto Begin2ColTable = [](const char* id) {
		return ImGui::BeginTable(id, 2, ImGuiTableFlags_SizingStretchProp | ImGuiTableFlags_PadOuterX);
		};

	// 4列用（2項目横並び: ラベル + 入力 + ラベル + 入力）
	auto Begin4ColTable = [](const char* id) {
		return ImGui::BeginTable(id, 4, ImGuiTableFlags_SizingStretchProp | ImGuiTableFlags_PadOuterX);
		};

	// --- 1. Basic Info ---
	if (ImGui::CollapsingHeader("Basic Info", ImGuiTreeNodeFlags_DefaultOpen))
	{
		if (Begin2ColTable("BasicInfoTable"))
		{
			// Name
			ImGui::TableNextRow();
			ImGui::TableSetColumnIndex(0);
			ImGui::TextUnformatted("Name");
			ImGui::TableSetColumnIndex(1);
			ImGui::SetNextItemWidth(-FLT_MIN);
			char nameBuffer[256];
			strncpy_s(nameBuffer, asset.info.base.name.c_str(), sizeof(nameBuffer));
			if (ImGui::InputText("##Name", nameBuffer, sizeof(nameBuffer)))
			{
				asset.info.base.name = nameBuffer;
			}

			// Type
			ImGui::TableNextRow();
			ImGui::TableSetColumnIndex(0);
			ImGui::TextUnformatted("Type");
			ImGui::TableSetColumnIndex(1);
			ImGui::TextDisabled("%s", asset.info.base.GetTypeName());

			// ID
			ImGui::TableNextRow();
			ImGui::TableSetColumnIndex(0);
			ImGui::TextUnformatted("ID");
			ImGui::TableSetColumnIndex(1);
			ImGui::Text("%u", asset.info.base.id);

			ImGui::EndTable();
		}
	}

	ImGui::Spacing();

	// --- 2. Spawn / Emitter ---
	if (ImGui::CollapsingHeader("Spawn / Emitter", ImGuiTreeNodeFlags_DefaultOpen))
	{
		if (Begin2ColTable("SpawnTable"))
		{
			int maxParticles = static_cast<int>(asset.cbDesc.param.maxParticles);

			// Emit Count
			ImGui::TableNextRow();
			ImGui::TableSetColumnIndex(0);
			ImGui::TextUnformatted("Emit Count");
			ImGui::TableSetColumnIndex(1);
			ImGui::SetNextItemWidth(-FLT_MIN);
			int emitCount = static_cast<int>(asset.cbDesc.param.emitCount);
			if (ImGui::DragInt("##EmitCount", &emitCount, 1, 1, maxParticles, "%d / s"))
			{
				asset.cbDesc.param.emitCount = static_cast<uint32_t>(emitCount);
				isChanged = true;
			}

			// Max Particles
			ImGui::TableNextRow();
			ImGui::TableSetColumnIndex(0);
			ImGui::TextUnformatted("Max Particles");
			ImGui::TableSetColumnIndex(1);
			ImGui::SetNextItemWidth(-FLT_MIN);
			if (ImGui::DragInt("##MaxParticles", &maxParticles, 100, 1, 100000))
			{
				asset.cbDesc.param.maxParticles = static_cast<uint32_t>(maxParticles);
				if (m_changeMaxParticleCallback) m_changeMaxParticleCallback(asset.info.base.id, asset.cbDesc.param.maxParticles);
			}

			// Life Time
			ImGui::TableNextRow();
			ImGui::TableSetColumnIndex(0);
			ImGui::TextUnformatted("Life Time");
			ImGui::TableSetColumnIndex(1);
			ImGui::SetNextItemWidth(-FLT_MIN);
			if (ImGui::DragFloat("##LifeTime", &asset.cbDesc.param.lifeTime, 0.1f, 0.01f, 60.0f, "%.2f s"))
			{
				isChanged = true;
			}

			ImGui::EndTable();
		}
	}

	ImGui::Spacing();

	// --- 3. Bounding Box ---
	if (ImGui::CollapsingHeader("Local AABB Bound"))
	{
		if (Begin2ColTable("AABBTable"))
		{
			ImGui::TableNextRow();
			ImGui::TableSetColumnIndex(0);
			ImGui::TextUnformatted("Min Bound");
			ImGui::TableSetColumnIndex(1);
			ImGui::SetNextItemWidth(-FLT_MIN);
			ImGui::DragFloat3("##MinBound", asset.cpuParam.minLocalAABB, 1.0f, -1000.0f, 0.0f, "%.1f");

			ImGui::TableNextRow();
			ImGui::TableSetColumnIndex(0);
			ImGui::TextUnformatted("Max Bound");
			ImGui::TableSetColumnIndex(1);
			ImGui::SetNextItemWidth(-FLT_MIN);
			ImGui::DragFloat3("##MaxBound", asset.cpuParam.maxLocalAABB, 1.0f, 0.0f, 1000.0f, "%.1f");

			asset.cpuParam.localAABB.min = DirectX::XMFLOAT3(asset.cpuParam.minLocalAABB[0], asset.cpuParam.minLocalAABB[1], asset.cpuParam.minLocalAABB[2]);
			asset.cpuParam.localAABB.max = DirectX::XMFLOAT3(asset.cpuParam.maxLocalAABB[0], asset.cpuParam.maxLocalAABB[1], asset.cpuParam.maxLocalAABB[2]);

			ImGui::EndTable();
		}
	}

	ImGui::Spacing();

	// --- 4. Particle Size (4列構成で横並び化) ---
	if (ImGui::CollapsingHeader("Size", ImGuiTreeNodeFlags_DefaultOpen))
	{
		if (Begin4ColTable("SizeTable"))
		{
			ImGui::TableNextRow();
			// Start Size
			ImGui::TableSetColumnIndex(0);
			ImGui::TextUnformatted("Start");
			ImGui::TableSetColumnIndex(1);
			ImGui::SetNextItemWidth(-FLT_MIN);
			if (ImGui::DragFloat("##StartSize", &asset.cbDesc.param.startSize, 0.05f, 0.0f, 100.0f, "%.2f"))
			{
				isChanged = true;
			}

			// End Size
			ImGui::TableSetColumnIndex(2);
			ImGui::TextUnformatted("End");
			ImGui::TableSetColumnIndex(3);
			ImGui::SetNextItemWidth(-FLT_MIN);
			if (ImGui::DragFloat("##EndSize", &asset.cbDesc.param.endSize, 0.05f, 0.0f, 100.0f, "%.2f"))
			{
				isChanged = true;
			}

			ImGui::EndTable();
		}
	}

	ImGui::Spacing();

	// --- 5. Movement & Physics ---
	if (ImGui::CollapsingHeader("Movement & Physics", ImGuiTreeNodeFlags_DefaultOpen))
	{
		if (Begin4ColTable("PhysicsTable"))
		{
			// 1行目: Gravity & Drag
			ImGui::TableNextRow();
			ImGui::TableSetColumnIndex(0);
			ImGui::TextUnformatted("Gravity");
			ImGui::TableSetColumnIndex(1);
			ImGui::SetNextItemWidth(-FLT_MIN);
			if (ImGui::DragFloat("##Gravity", &asset.cbDesc.param.gravity, 0.0001f, -10.0f, 10.0f, "%.4f"))
			{
				isChanged = true;
			}

			ImGui::TableSetColumnIndex(2);
			ImGui::TextUnformatted("Drag");
			ImGui::TableSetColumnIndex(3);
			ImGui::SetNextItemWidth(-FLT_MIN);
			if (ImGui::DragFloat("##Drag", &asset.cbDesc.param.drag, 0.01f, 0.0f, 10.0f, "%.2f"))
			{
				isChanged = true;
			}

			// 2行目: Turbulence Noise
			ImGui::TableNextRow();
			ImGui::TableSetColumnIndex(0);
			ImGui::TextUnformatted("Turbulence");
			ImGui::TableSetColumnIndex(1);
			ImGui::SetNextItemWidth(-FLT_MIN);
			if (ImGui::DragFloat("##Noise", &asset.cbDesc.param.noiseStrength, 0.01f, 0.0f, 10.0f, "%.2f"))
			{
				isChanged = true;
			}

			ImGui::EndTable();
		}
	}

	ImGui::Spacing();

	// --- 6. Color & Material ---
	if (ImGui::CollapsingHeader("Color & Material", ImGuiTreeNodeFlags_DefaultOpen))
	{
		if (Begin4ColTable("ColorTable"))
		{
			ImGui::TableNextRow();
			// Start Color
			ImGui::TableSetColumnIndex(0);
			ImGui::TextUnformatted("Start");
			ImGui::TableSetColumnIndex(1);
			ImGui::SetNextItemWidth(-FLT_MIN);
			if (ImGui::ColorEdit4("##StartColor", asset.cbDesc.param.startColor, ImGuiColorEditFlags_AlphaBar | ImGuiColorEditFlags_Float))
			{
				isChanged = true;
			}

			// End Color
			ImGui::TableSetColumnIndex(2);
			ImGui::TextUnformatted("End");
			ImGui::TableSetColumnIndex(3);
			ImGui::SetNextItemWidth(-FLT_MIN);
			if (ImGui::ColorEdit4("##EndColor", asset.cbDesc.param.endColor, ImGuiColorEditFlags_AlphaBar | ImGuiColorEditFlags_Float))
			{
				isChanged = true;
			}

			ImGui::EndTable();
		}

		if (Begin2ColTable("MaterialTable"))
		{
			// Emissive Intensity
			ImGui::TableNextRow();
			ImGui::TableSetColumnIndex(0);
			ImGui::TextUnformatted("Emissive");
			ImGui::TableSetColumnIndex(1);
			ImGui::SetNextItemWidth(-FLT_MIN);
			if (ImGui::DragFloat("##Emissive", &asset.cbDesc.param.emissive, 0.1f, 0.0f, 100.0f, "%.1f"))
			{
				isChanged = true;
			}

			// Blend Mode
			ImGui::TableNextRow();
			ImGui::TableSetColumnIndex(0);
			ImGui::TextUnformatted("Blend Mode");
			ImGui::TableSetColumnIndex(1);
			ImGui::SetNextItemWidth(-FLT_MIN);
			const char* blendNames[] = { "Opaque", "AlphaBlend", "Additive", "Subtractive" };
			int currentBlend = static_cast<int>(asset.cpuParam.blend);
			if (ImGui::Combo("##BlendMode", &currentBlend, blendNames, IM_ARRAYSIZE(blendNames)))
			{
				asset.cpuParam.blend = static_cast<GraphicsPipeline::BlendMode>(currentBlend);
				isChanged = true; // 必要に応じて変更フラグ
			}

			ImGui::EndTable();
		}
	}

	if (isChanged)
	{
		asset.gpuResource.cb.Upload(asset.cbDesc);
	}
}