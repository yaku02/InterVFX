#pragma once

class RootSignatureManager {
	template <typename T>
	using ComPtr = Microsoft::WRL::ComPtr<T>;


private:
	inline static ComPtr<ID3D12RootSignature> m_rootSignature = nullptr;
	Microsoft::WRL::ComPtr<ID3D12RootSignature> CreateRootSignature(ID3D12Device* dev, D3D12_ROOT_SIGNATURE_DESC& desc);

public:
	~RootSignatureManager()
	{
		m_rootSignature.Reset();
		OutputDebugStringA("RootSigManager Destroy\n");
	}


	void Init(ID3D12Device* dev) {
		CreateCommonRootSignature(dev);
	}

	HRESULT CreateCommonRootSignature(ID3D12Device* dev);
	static ComPtr<ID3D12RootSignature>& GetRootSignature() { return m_rootSignature; }


};