#pragma once

class ComputePipeline {
public:
    struct Desc {
        // --- シェーダー関連 ---
        std::string csPath = "";
        std::string csEntry = "CSMain";
        std::string csModel = "cs_6_6";

        // --- ルートシグネチャ ---
        Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature = nullptr;

        // --- ネイティブ構造体（詳細設定用） ---
        D3D12_COMPUTE_PIPELINE_STATE_DESC d3dDesc{};

        // コンストラクタでデフォルト値を設定
        Desc() {
            d3dDesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;
            d3dDesc.NodeMask = 0;
        }
    };

private:
	Microsoft::WRL::ComPtr<ID3D12PipelineState> pipelineState = nullptr;
	Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature = nullptr;

public:
	static ComputePipeline Create(ID3D12Device* dev, const Desc& desc);

	void SetPipeline(ID3D12GraphicsCommandList* cmdList) {
		cmdList->SetPipelineState(pipelineState.Get());
		cmdList->SetComputeRootSignature(rootSignature.Get());
	}
};