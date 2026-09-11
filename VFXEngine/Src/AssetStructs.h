#pragma once

enum class AssetType {
	VFX,
	Unkown,
};
struct AssetInfo {
	AssetType type = AssetType::Unkown;
	uint32_t id = UINT32_MAX;
	std::string name = "unknown_asset";
	D3D12_GPU_DESCRIPTOR_HANDLE iconSrvHandle{};
};