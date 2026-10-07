#pragma once

#include "IDGenerator.h"
#include "Geometry.h"
#include "InstanceStructs.h"
#include "ConstantBuffer.h"
#include "VFXStructs.h"

class VFXAsset;
class GpuResourceManager;

class VFXInstance {
public:
    struct Info {
        inline static uint32_t Counter = 0;
        InstBaseInfo base{};
        uint32_t assetID = IDGenerator::INVALID_ID;
        std::string assetName = "";
    }info{};

    struct CPUParam {
        DirectX::XMFLOAT3 position = { 0.0f, 0.0f, 0.0f };
        DirectX::XMFLOAT3 rotation = { 0.0f, 0.0f, 0.0f };
        DirectX::XMFLOAT3 scale = { 1.0f, 1.0f, 1.0f };

        DirectX::XMMATRIX world = DirectX::XMMatrixIdentity();

        bool isVisible = true;
        bool isActive = true;

        AABB worldAABB{};

    }cpuParam{};

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
        D3D12_GPU_DESCRIPTOR_HANDLE particleSRVHandle{}; // プレビュー用

        Microsoft::WRL::ComPtr<ID3D12Resource> uploadBuffer = nullptr; // Upload用の一時的バッファ ExecuteCommandListが行われるまで保持

    }gpuResource{};

    void UploadConstBuff()
    {
        cbDesc.param.transform =
            DirectX::XMMatrixTranspose(cpuParam.world);
        gpuResource.cb.Upload(cbDesc);
    }

	const uint32_t GetCBVIndex() const { return gpuResource.cb.descriptorIndex; }
public:
	static std::shared_ptr<VFXInstance> Create(VFXAsset& asset, ID3D12Device* dev, ID3D12GraphicsCommandList* cmdList, GpuResourceManager& gpuResMgr);
	HRESULT CreateGpuResource(VFXAsset& asset, ID3D12Device* dev, ID3D12GraphicsCommandList* cmdList, GpuResourceManager& gpuResMgr);
};