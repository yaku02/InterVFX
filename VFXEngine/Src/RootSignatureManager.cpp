#include "pch.h"
#include "RootSignatureManager.h"
#include "Debugger.h"
#include "RootParamLayout.h"

HRESULT RootSignatureManager::CreateCommonRootSignature(ID3D12Device* dev)
{
	CD3DX12_ROOT_PARAMETER rootParam{};
	rootParam.InitAsConstants(
		static_cast<UINT>(RootParam::CBVIndices::Count),
		0, // register(b0)
		0, // space0
		D3D12_SHADER_VISIBILITY_ALL
	);

	CD3DX12_STATIC_SAMPLER_DESC staticSamplers[] = {
		// s0: Point (既存)
		CD3DX12_STATIC_SAMPLER_DESC(0, D3D12_FILTER_MIN_MAG_MIP_POINT),

		// 今後増やす例（コメントアウトを外すか追記）：
		// CD3DX12_STATIC_SAMPLER_DESC(1, D3D12_FILTER_MIN_MAG_MIP_LINEAR), // s1: Linear
	};

	D3D12_ROOT_SIGNATURE_DESC rsDesc = {};
	rsDesc.NumParameters = static_cast<UINT>(RootParam::Slot::Count);
	rsDesc.pParameters = &rootParam;
	rsDesc.NumStaticSamplers = static_cast<UINT>(_countof(staticSamplers));
	rsDesc.pStaticSamplers = staticSamplers;
	rsDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_NONE
		| D3D12_ROOT_SIGNATURE_FLAG_CBV_SRV_UAV_HEAP_DIRECTLY_INDEXED;

	m_rootSignature = CreateRootSignature(dev, rsDesc);
	if (!m_rootSignature) {
		Debugger::Log("[Error] Creattion Common rootSignature failed\n");
		return E_FAIL;
	}

	m_rootSignature->SetName(L"Common Root Signature");
	return S_OK;
}


Microsoft::WRL::ComPtr<ID3D12RootSignature> RootSignatureManager::CreateRootSignature(
	ID3D12Device* dev,
	D3D12_ROOT_SIGNATURE_DESC& desc)
{
	Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSig;
	Microsoft::WRL::ComPtr<ID3DBlob> sigBlob, errBlob;

	HRESULT hr = D3D12SerializeRootSignature(&desc, D3D_ROOT_SIGNATURE_VERSION_1_0, &sigBlob, &errBlob);
	if (FAILED(hr)) {
		if (errBlob) {
			OutputDebugStringA((char*)errBlob->GetBufferPointer());
		}
		return nullptr;
	}

	hr = dev->CreateRootSignature(0, sigBlob->GetBufferPointer(), sigBlob->GetBufferSize(), IID_PPV_ARGS(&rootSig));
	if (FAILED(hr)) {
		Debugger::Log("[Error] Failed to CreateRootSignature on device.\n");
		return nullptr;
	}

	return rootSig;
}