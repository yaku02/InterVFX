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
            uint32_t deadListSRV = UINT32_MAX;
            uint32_t deadListUAV = UINT32_MAX;
            uint32_t activeListSRV = UINT32_MAX;
            uint32_t activeListUAV = UINT32_MAX; // UpdateCSで Append する場合
            uint32_t activeCounterUAV = UINT32_MAX;
            uint32_t indirectArgsUAV = UINT32_MAX; // 引数バッファをGPUで書き換える場合（またはSRVとして描画時に読む）
        }descIndices{};
    }cbDesc{};

    struct GPUResource {
        ConstantBuffer<CBDesc> cb;

        Microsoft::WRL::ComPtr<ID3D12Resource> particleBuffer = nullptr;
        Microsoft::WRL::ComPtr<ID3D12Resource> uploadBuffer = nullptr; // Upload用の一時的バッファ ExecuteCommandListが行われるまで保持

        Microsoft::WRL::ComPtr<ID3D12Resource> deadListBuffer = nullptr;
        Microsoft::WRL::ComPtr<ID3D12Resource> deadUploadBuffer = nullptr;
        Microsoft::WRL::ComPtr<ID3D12Resource> deadCounterBuffer = nullptr;
        Microsoft::WRL::ComPtr<ID3D12Resource> counterUploadBuffer = nullptr;
        
        Microsoft::WRL::ComPtr<ID3D12Resource> activeListBuffer = nullptr;    // 生存インデックス用
        Microsoft::WRL::ComPtr<ID3D12Resource> activeListUploadBuffer = nullptr;
        Microsoft::WRL::ComPtr<ID3D12Resource> activeCounterBuffer = nullptr; // 生存数カウンター用
        Microsoft::WRL::ComPtr<ID3D12Resource> activeCountUploadBuffer = nullptr;

        Microsoft::WRL::ComPtr<ID3D12Resource> indirectArgsBuffer = nullptr;  // DrawInstancedIndirect用引数
        Microsoft::WRL::ComPtr<ID3D12Resource> argsUploadBuffer = nullptr;

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

private:
	HRESULT CreateGpuResource(VFXAsset& asset, ID3D12Device* dev, ID3D12GraphicsCommandList* cmdList, GpuResourceManager& gpuResMgr);
    HRESULT CreateParticleBuffer(VFXAsset& asset, ID3D12Device* dev, ID3D12GraphicsCommandList* cmdList, GpuResourceManager& gpuResMgr);
    HRESULT CreateDeadListBuffer(VFXAsset& asset, ID3D12Device* dev, ID3D12GraphicsCommandList* cmdList, GpuResourceManager& gpuResMgr);
    HRESULT CreateActiveListBuffer(VFXAsset& asset, ID3D12Device* dev, ID3D12GraphicsCommandList* cmdList, GpuResourceManager& gpuResMgr);
    HRESULT CreateIndirectArgsBuffer(VFXAsset& asset, ID3D12Device* dev, ID3D12GraphicsCommandList* cmdList, GpuResourceManager& gpuResMgr);
};