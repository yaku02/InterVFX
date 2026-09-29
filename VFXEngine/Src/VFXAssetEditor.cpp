#include "pch.h"
#include "VFXAssetEditor.h"
#include "VFXAsset.h"
#include "IDGenerator.h"
#include "UIManager.h"

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

	bool isChanged = false;

	// 2列用（標準）
	auto Begin2ColTable = [](const char* id) {
		return ImGui::BeginTable(id, 2, ImGuiTableFlags_SizingStretchProp | ImGuiTableFlags_PadOuterX);
		};

	// 4列用（2項目横並び）
	auto Begin4ColTable = [](const char* id) {
		return ImGui::BeginTable(id, 4, ImGuiTableFlags_SizingStretchProp | ImGuiTableFlags_PadOuterX);
		};

	// --- 1. Basic Info ---
	if (ImGui::CollapsingHeader("Basic Info", ImGuiTreeNodeFlags_DefaultOpen))
	{
		if (Begin2ColTable("BasicInfoTable"))
		{
			// Name
			ImGui::TableNextColumn();
			ImGui::TextUnformatted("Name");
			ImGui::TableNextColumn();
			ImGui::SetNextItemWidth(-FLT_MIN);
			char nameBuffer[256];
			strncpy_s(nameBuffer, asset.info.base.name.c_str(), sizeof(nameBuffer));
			if (ImGui::InputText("##Name", nameBuffer, sizeof(nameBuffer)))
			{
				asset.info.base.name = nameBuffer;
			}

			// Type
			ImGui::TableNextColumn();
			ImGui::TextUnformatted("Type");
			ImGui::TableNextColumn();
			ImGui::TextDisabled("%s", asset.info.base.GetTypeName());

			// ID
			ImGui::TableNextColumn();
			ImGui::TextUnformatted("ID");
			ImGui::TableNextColumn();
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
			// Spawn Rate
			ImGui::TableNextColumn();
			ImGui::TextUnformatted("Spawn Rate");
			ImGui::TableNextColumn();
			ImGui::SetNextItemWidth(-FLT_MIN);
			ImGui::DragFloat("##SpawnRate", &asset.cpuParam.spawnRate, 1.0f, 0.0f, 10000.0f, "%.1f / s");

			// Max Particles
			ImGui::TableNextColumn();
			ImGui::TextUnformatted("Max Particles");
			ImGui::TableNextColumn();
			ImGui::SetNextItemWidth(-FLT_MIN);
			int maxParticles = static_cast<int>(asset.gpuParam.numParticles);
			if (ImGui::DragInt("##MaxParticles", &maxParticles, 100, 1, 1000000))
			{
				asset.gpuParam.numParticles = static_cast<uint32_t>(maxParticles);
				isChanged = true;
			}

			// Life Time
			ImGui::TableNextColumn();
			ImGui::TextUnformatted("Life Time");
			ImGui::TableNextColumn();
			ImGui::SetNextItemWidth(-FLT_MIN);
			if (ImGui::DragFloat("##LifeTime", &asset.gpuParam.lifeTime, 0.1f, 0.01f, 60.0f, "%.2f s"))
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
			ImGui::TableNextColumn();
			ImGui::TextUnformatted("Min Bound");
			ImGui::TableNextColumn();
			ImGui::SetNextItemWidth(-FLT_MIN);
			ImGui::DragFloat3("##MinBound", asset.cpuParam.minLocalAABB, 1.0f, -1000.0f, 0.0f, "%.1f");

			ImGui::TableNextColumn();
			ImGui::TextUnformatted("Max Bound");
			ImGui::TableNextColumn();
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
			// Start Size
			ImGui::TableNextColumn();
			ImGui::TextUnformatted("Start");
			ImGui::TableNextColumn();
			ImGui::SetNextItemWidth(-FLT_MIN);
			if (ImGui::DragFloat("##StartSize", &asset.gpuParam.startSize, 0.05f, 0.0f, 100.0f, "%.2f"))
			{
				isChanged = true;
			}

			// End Size
			ImGui::TableNextColumn();
			ImGui::TextUnformatted("End");
			ImGui::TableNextColumn();
			ImGui::SetNextItemWidth(-FLT_MIN);
			if (ImGui::DragFloat("##EndSize", &asset.gpuParam.endSize, 0.05f, 0.0f, 100.0f, "%.2f"))
			{
				isChanged = true;
			}

			ImGui::EndTable();
		}
	}

	ImGui::Spacing();

	// --- 5. Movement & Physics (4列構成＋一部2列) ---
	if (ImGui::CollapsingHeader("Movement & Physics", ImGuiTreeNodeFlags_DefaultOpen))
	{
		if (Begin4ColTable("PhysicsTable"))
		{
			// 1行目: Gravity & Drag
			ImGui::TableNextColumn();
			ImGui::TextUnformatted("Gravity");
			ImGui::TableNextColumn();
			ImGui::SetNextItemWidth(-FLT_MIN);
			if (ImGui::DragFloat("##Gravity", &asset.gpuParam.gravity, 0.0001f, -10.0f, 10.0f, "%.4f"))
			{
				isChanged = true;
			}

			ImGui::TableNextColumn();
			ImGui::TextUnformatted("Drag");
			ImGui::TableNextColumn();
			ImGui::SetNextItemWidth(-FLT_MIN);
			if (ImGui::DragFloat("##Drag", &asset.gpuParam.drag, 0.01f, 0.0f, 10.0f, "%.2f"))
			{
				isChanged = true;
			}

			// 2行目: Turbulence Noise (残り2列分を跨いで配置)
			ImGui::TableNextColumn();
			ImGui::TextUnformatted("Turbulence");
			ImGui::TableNextColumn();
			ImGui::SetNextItemWidth(-FLT_MIN);
			if (ImGui::DragFloat("##Noise", &asset.gpuParam.noiseStrength, 0.01f, 0.0f, 10.0f, "%.2f"))
			{
				isChanged = true;
			}

			// 空白埋め用
			ImGui::TableNextColumn();
			ImGui::TableNextColumn();

			ImGui::EndTable();
		}
	}

	ImGui::Spacing();

	// --- 6. Color & Material ---
	if (ImGui::CollapsingHeader("Color & Material", ImGuiTreeNodeFlags_DefaultOpen))
	{
		// カラーは2列でラベルとColorEditを横並びにする（Start/Endで2行）
		if (Begin4ColTable("ColorTable"))
		{
			// Start Color
			ImGui::TableNextColumn();
			ImGui::TextUnformatted("Start");
			ImGui::TableNextColumn();
			ImGui::SetNextItemWidth(-FLT_MIN);
			if (ImGui::ColorEdit4("##StartColor", asset.gpuParam.startColor, ImGuiColorEditFlags_AlphaBar | ImGuiColorEditFlags_Float))
			{
				isChanged = true;
			}

			// End Color
			ImGui::TableNextColumn();
			ImGui::TextUnformatted("End");
			ImGui::TableNextColumn();
			ImGui::SetNextItemWidth(-FLT_MIN);
			if (ImGui::ColorEdit4("##EndColor", asset.gpuParam.endColor, ImGuiColorEditFlags_AlphaBar | ImGuiColorEditFlags_Float))
			{
				isChanged = true;
			}

			ImGui::EndTable();
		}

		if (Begin2ColTable("MaterialTable"))
		{
			// Emissive Intensity
			ImGui::TableNextColumn();
			ImGui::TextUnformatted("Emissive");
			ImGui::TableNextColumn();
			ImGui::SetNextItemWidth(-FLT_MIN);
			if (ImGui::DragFloat("##Emissive", &asset.gpuParam.emissive, 0.1f, 0.0f, 100.0f, "%.1f"))
			{
				isChanged = true;
			}

			// Blend Mode
			ImGui::TableNextColumn();
			ImGui::TextUnformatted("Blend Mode");
			ImGui::TableNextColumn();
			ImGui::SetNextItemWidth(-FLT_MIN);
			const char* blendNames[] = { "Opaque", "AlphaBlend", "Additive", "Subtractive" };
			int currentBlend = static_cast<int>(asset.cpuParam.blend);
			if (ImGui::Combo("##BlendMode", &currentBlend, blendNames, IM_ARRAYSIZE(blendNames)))
			{
				asset.cpuParam.blend = static_cast<BlendMode>(currentBlend);
			}

			ImGui::EndTable();
		}
	}

	if (isChanged)
	{
		asset.UploadConstBuff();
	}
}