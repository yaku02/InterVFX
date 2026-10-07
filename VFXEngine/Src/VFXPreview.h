#pragma once
#include "ConstantBuffer.h"

class VFXAsset;
class GpuResourceManager;
class VFXPreview {
public:
	static VFXPreview SetAsset(VFXAsset& asset, ID3D12Device* dev, ID3D12GraphicsCommandList* cmdList, GpuResourceManager& gpuResMgr);

	std::shared_ptr<VFXAsset> m_targetAsset = nullptr;

    struct CBDesc {
        struct Param {
            DirectX::XMMATRIX transform = DirectX::XMMatrixIdentity();
            DirectX::XMFLOAT3 direction = { 0.0f, 1.0f, 0.0f };
            float speed = 1.0f;
        }param{};

        struct DescIndices {
            uint32_t particleSRV = UINT32_MAX;
            uint32_t particleUAV = UINT32_MAX;
            float padding[2];
        }descIndices{};
    }cbDesc{};

    struct GPUResource {
        ConstantBuffer<CBDesc> cb;

        Microsoft::WRL::ComPtr<ID3D12Resource> particleBuffer = nullptr;
        Microsoft::WRL::ComPtr<ID3D12Resource> uploadBuffer = nullptr; // Upload用の一時的バッファ ExecuteCommandListが行われるまで保持

    }gpuResource{};

    void UploadConstBuff()
    {
        cbDesc.param.transform =
            DirectX::XMMatrixTranspose(cbDesc.param.transform);
        gpuResource.cb.Upload(cbDesc);
    }

private:
    HRESULT CreateGpuResource(VFXAsset& asset, ID3D12Device* dev, ID3D12GraphicsCommandList* cmdList, GpuResourceManager& gpuResMgr);

};