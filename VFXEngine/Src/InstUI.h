#pragma once
#include "InstanceStructs.h"

class InstUI {
	using SelectCallback = std::function<void(uint32_t instID)>;
	using DeleteCallback = std::function<HRESULT(uint32_t id)>;
	using DnDCreateCallback = std::function<HRESULT(uint32_t assetID)>;

private:
	inline static std::unordered_map<InstType, DnDCreateCallback> m_dndCreateCallbacks{};
	inline static std::unordered_map<InstType, SelectCallback> m_selectCallbacks{};
	inline static std::unordered_map<InstType, DeleteCallback> m_deleteCallbacks{};

	inline static std::vector<InstBaseInfo> m_insts;


	InstBaseInfo m_selectedInst = {};
	InstBaseInfo m_deleteTarget = {};

	void BeginInstPopup(const InstBaseInfo& inst);
	void ExecuteDeletion();
	void HandleDragDropTarget();

public:
	void ShowUI();

	static void Add(const InstBaseInfo& info);

	static void RegisterDnDCreateCallback(InstType type, DnDCreateCallback callback);
	static void RegisterSelectCallback(InstType type, SelectCallback callback);
	static void RegisterDeleteCallback(InstType type, DeleteCallback callBack);

};