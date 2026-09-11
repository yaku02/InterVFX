#pragma once

enum class BlendMode {
	Alpha,
	Additive,
};

struct PipelineObject
{
	Microsoft::WRL::ComPtr<ID3D12PipelineState> pipelineState = nullptr;
	Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature = nullptr;

	void SetGraphicsPipeline(ID3D12GraphicsCommandList* cmdList) {
		cmdList->SetPipelineState(pipelineState.Get());
		cmdList->SetGraphicsRootSignature(rootSignature.Get());
	}

	void SetComputePipeline(ID3D12GraphicsCommandList* cmdList) {
		cmdList->SetPipelineState(pipelineState.Get());
		cmdList->SetComputeRootSignature(rootSignature.Get());
	}
};

struct ComputePSODesc {
	std::string csPath = "";
	std::string csEntry = "";
	Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature = nullptr;
	std::string csModel = "cs_6_6";
};

struct GraphicsPSODesc {
	std::string vsPath;                 // VSのファイルパス
	std::string vsEntry;                // VSのエントリーポイント (例: "VSMain")
	std::string psPath;                 // PSのファイルパス
	std::string psEntry;                // PSのエントリーポイント (例: "PSMain")
	Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature = nullptr;
	DXGI_FORMAT rtvFormat;                      // レンダ―ターゲットのフォーマット (例: DXGI_FORMAT_R8G8B8A8_UNORM)
	DXGI_FORMAT dsvFormat;                      // 深度バッファのフォーマット (例: DXGI_FORMAT_D32_FLOAT)
	D3D12_PRIMITIVE_TOPOLOGY_TYPE topologyType; // プリミティブ種別 (通常: D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE)
	bool isDepthWrite;                          // 深度バッファへの書き込みを行うか (VFXの半透明は通常false)
	BlendMode blendMode = BlendMode::Alpha;     // 通常のアルファ半透明 (Dest: INV_SRC_ALPHA)　// 加算合成 (Dest: ONE)
	std::string vsModel = "vs_6_6";
	std::string psModel = "ps_6_6";
};