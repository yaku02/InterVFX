#pragma once

class GraphicsPipeline {
public:
	enum class BlendMode {
		Alpha = 0,    // 通常のアルファ半透明 (Dest: INV_SRC_ALPHA)
		Additive = 1, // 加算合成 (Dest: ONE)
	};

    struct Desc {
        // --- 独自管理：シェーダーパス情報 ---
        std::string vsPath = "";
        std::string vsEntry = "VSMain";
        std::string vsModel = "vs_6_6";

        std::string psPath = "";
        std::string psEntry = "PSMain";
        std::string psModel = "ps_6_6";

        // --- 独自管理：簡易設定用 ---
        BlendMode blendMode = BlendMode::Alpha;
		Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature = nullptr;

        // --- ネイティブ構造体を内包して直接細かく設定も可能にする ---
        D3D12_GRAPHICS_PIPELINE_STATE_DESC d3dDesc{};

        // コンストラクタで「よく使う安全なデフォルト値」を自動セットしておく
        Desc() {
            d3dDesc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
            d3dDesc.RasterizerState.CullMode = D3D12_CULL_MODE_BACK;
            d3dDesc.DepthStencilState.DepthEnable = TRUE;
            d3dDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
            d3dDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
            d3dDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
            d3dDesc.NumRenderTargets = 1;
            d3dDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
            d3dDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
            d3dDesc.SampleDesc.Count = 1;
            d3dDesc.SampleMask = UINT_MAX;
        }
    };

private:
	Microsoft::WRL::ComPtr<ID3D12PipelineState> pipelineState = nullptr;
	Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature = nullptr;
public:
	static GraphicsPipeline Create(ID3D12Device* dev, const Desc& desc);

	void SetPipeline(ID3D12GraphicsCommandList* cmdList) {
		cmdList->SetPipelineState(pipelineState.Get());
		cmdList->SetGraphicsRootSignature(rootSignature.Get());
	}
};