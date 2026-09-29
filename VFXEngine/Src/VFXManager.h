#pragma once
#include "VFXAsset.h"
#include "VFXInstance.h"

class Window;
class Dx12Wrapper;
class GpuResourceManager;
class TextureManager;
class VFXPipeline;
class VFXAssetEditor;
class VFXInstEditor;

class VFXManager {
private:
	Window* m_window = nullptr;
	Dx12Wrapper* m_dx12 = nullptr;
	GpuResourceManager* m_gpuResMgr = nullptr;
	TextureManager* m_texMgr = nullptr;

	std::unique_ptr<VFXPipeline> m_pipeline = nullptr;
	std::unique_ptr<VFXAssetEditor> m_assetEditor = nullptr;
	std::unique_ptr<VFXInstEditor> m_instEditor = nullptr;

	std::vector<std::shared_ptr<VFXAsset>> m_assets{};
	std::vector<std::shared_ptr<VFXInstance>> m_insts{};

	std::unordered_map<uint32_t, std::shared_ptr<VFXAsset>> m_assetsMap{};
	std::unordered_map<uint32_t, std::shared_ptr<VFXInstance>> m_instsMap{};

	Texture m_defaultAssetIcon{};

	void OnAssetSelected(const uint32_t assetID);
	void OpenAssetEditor(const uint32_t assetID);

	void OnInstSelected(const uint32_t instID);
	void OpenInstEditor(const uint32_t instID);
public:
	VFXManager();
	~VFXManager();

	struct InitDesc {
		Window& window;
		Dx12Wrapper& dx12;
		GpuResourceManager& gpuResMgr;
		TextureManager& texMgr;
	};

	void Init(InitDesc& desc);
	void ShutDown();

	VFXAsset* GetAsset(const uint32_t assetID);
	VFXInstance* GetInst(const uint32_t instID);

	VFXAssetEditor* GetAssetEditor() {
		return m_assetEditor ? m_assetEditor.get() : nullptr;
	}

	VFXInstEditor* GetInstEditor() {
		return m_instEditor ? m_instEditor.get() : nullptr;
	}

	HRESULT CreateAndRegisterAsset();
	HRESULT CreateAndRegisterInstance(const uint32_t assetID);

	bool DeleteAsset(const uint32_t assetID);
	bool DeleteInst(const uint32_t instID);
};