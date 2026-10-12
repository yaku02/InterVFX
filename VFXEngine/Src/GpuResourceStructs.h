#pragma once

enum class HeapType {
	Visible,
	NonVisible,
	ImGui,
	Rtv,
	Dsv,
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
		default: return Type::None; // 範囲外は None として扱う
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

enum class ResState {
	Common,                 // 初期状態や汎用的な状態 (D3D12_RESOURCE_STATE_COMMON)
	VertexAndConstantBuffer,// 頂点バッファ、定数バッファ (D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER)
	IndexBuffer,            // インデックスバッファ (D3D12_RESOURCE_STATE_INDEX_BUFFER)
	RenderTarget,           // レンダーターゲット (D3D12_RESOURCE_STATE_RENDER_TARGET)
	UnorderedAccess,        // UAV（コンピュートシェーダーからの書き込みなど） (D3D12_RESOURCE_STATE_UNORDERED_ACCESS)
	NonPixelShaderResource, // SRV（VSやCSでテクスチャ・バッファを読むとき） (D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE)
	PixelShaderResource,    // SRV（ピクセルシェーダーでテクスチャを読むとき） (D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE)
	IndirectArgument,       // ExecuteIndirect の引数バッファ (D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT)
	CopyDest,               // CopyBufferRegion などのコピー先 (D3D12_RESOURCE_STATE_COPY_DEST)
	CopySource,             // CopyBufferRegion などのコピー元 (D3D12_RESOURCE_STATE_COPY_SOURCE)
	Present,                // 画面表示用（スワップチェーン） (D3D12_RESOURCE_STATE_PRESENT)
	DepthWrite,             // 深度ステンシル（書き込み） (D3D12_RESOURCE_STATE_DEPTH_WRITE)
	DepthRead               // 深度ステンシル（読み取り） (D3D12_RESOURCE_STATE_DEPTH_READ)
};