#pragma once
#include "TextureStructs.h"
#include "IDGenerator.h"
#include "InstanceStructs.h"

enum class AssetType {
	VFX,
	Unkown,
};

struct AssetBaseInfo {
	AssetType type = AssetType::Unkown;
	InstType instType = InstType::Unknown;

	uint32_t id = IDGenerator::INVALID_ID;
	std::string name = "unknown_asset";
	Texture icon{};

	const char* GetTypeName() const {
		if (type == AssetType::VFX) { return "VFX"; }
		else { return "Unknown";}
	}
};