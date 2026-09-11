#pragma once

enum class HeapType {
	Visible,
	NonVisible,
	ImGui,
	Rtv,
};

struct DescHeapInfo {
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> heap = nullptr;
	D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle{};
	D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle{};
	UINT currentIdx = 0;
};


struct Descriptor {
	enum class Type {
		Unknown,
		CBV,
		SRV,
		UAV
	}type = Type::CBV;

	uint32_t viewIndex = UINT32_MAX;

	const char* GetHLSLTypeName() const {
		switch (type) {
		case Type::CBV:       return "ConstantBuffer";
		case Type::SRV:       return "StructuredBuffer";
		case Type::UAV:       return "RWStructuredBuffer";
		default:                      return "Unknown";
		}
	}
	const char* GetCharFromType() const {
		switch (type) {
		case Type::CBV: return "CBV";
		case Type::SRV: return "SRV";
		case Type::UAV: return "UAV";
		default: return "Unknown";
		}
	}

	Type GetTypeFromChar(const char* typeName) {
		if (typeName == "CBV") return Type::CBV;
		else if (typeName == "SRV") return Type::SRV;
		else if (typeName == "UAV") return Type::UAV;
		else return Type::Unknown;
	}

};

struct UploadFrequency {
	enum class Type {
		PerFrame,
		PerAsset,
		PerInstance,
	}type = Type::PerAsset;

	const char* GetChar() const {
		switch (type) {
		case Type::PerFrame:		 return "Frame";
		case Type::PerAsset:	     return "Asset";
		case Type::PerInstance:      return "Inst";
		default:                     return "Unknown";
		}
	}
};


struct HeapPropInfo {
	enum class Type {
		None = 0,
		Upload,
		Default,
	}type = Type::None;

	inline Type GetTypeFromIndex(int index) {
		switch (index) {
		case 0:  return Type::None;
		case 1:  return Type::Upload;
		case 2:  return Type::Default;
		default: return Type::None; // ”ÍˆÍŠO‚Í None ‚Æ‚µ‚Äˆµ‚¤
		}
	}
};

enum class ShaderVisibleType {
	None,
	All,
	Compute,
	Vertex,
	Pixel
};