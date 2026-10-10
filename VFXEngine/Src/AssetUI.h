#pragma once
#include "AssetStructs.h"

class VFXManager;
class AssetUI {
	using CreationCallback = std::function<HRESULT()>;
	using DeleteCallback = std::function<HRESULT(uint32_t id)>;
	using OpenEditorCallback = std::function<void(uint32_t id)>;
	using SelectCallback = std::function<void(uint32_t assetID)>; 

private:
	inline static std::vector<AssetBaseInfo> m_assets;
	inline static std::unordered_map<AssetType, CreationCallback> m_creationCallbacks{};
	inline static std::unordered_map<AssetType, DeleteCallback> m_deleteCallbacks{};
	inline static std::unordered_map<AssetType, SelectCallback> m_selectCallbacks{};

	AssetBaseInfo m_selectedAsset = {};
	AssetBaseInfo m_deleteTarget = {};

public:
	~AssetUI() = default;

	struct InitDesc {
	};

	void Init(const InitDesc& desc);
	void ShutDown();

	void ShowUI();
	static void Add(const AssetBaseInfo& info);
	static void RegisterCreationCallback(AssetType type, CreationCallback callback);
	static void RegisterDeleteCallback(AssetType type, DeleteCallback callback);
	static void RegisterSelectCallback(AssetType type, SelectCallback callback);

	void BeginWindowPopup();
	void BeginAssetPopup(const AssetBaseInfo& asset);
	void ExecuteDeletion();


};