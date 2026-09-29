#pragma once
#include "IDGenerator.h"

enum class InstType {
	VFX,
	Unknown
};

struct InstBaseInfo {
	InstType type = InstType::Unknown;
	uint32_t id = IDGenerator::INVALID_ID;
	std::string name = "unkown_instance";
};