#pragma once

#include "IDGenerator.h"
#include "Geometry.h"
#include "InstanceStructs.h"

class VFXAsset;
class Dx12Wrapper;
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

    struct GPUParam {
        struct {
            DirectX::XMMATRIX world = DirectX::XMMatrixIdentity();
        } transform;

        struct {
            DirectX::XMFLOAT3 direction = { 0.0f, 1.0f, 0.0f };
            float speed = 1.0f;
        } behavior;

        struct Indices {
            uint32_t srvIndex = UINT32_MAX;
            uint32_t uavIndex = UINT32_MAX;
            float padding[2];
        } indices;

    }gpuParam{};

    struct GPUResource {
        Microsoft::WRL::ComPtr<ID3D12Resource> particleBuffer = nullptr;
        D3D12_GPU_DESCRIPTOR_HANDLE particleSRVHandle{}; // プレビュー用

        Microsoft::WRL::ComPtr<ID3D12Resource> uploadBuffer = nullptr; // Upload用の一時的バッファ ExecuteCommandListが行われるまで保持
        Microsoft::WRL::ComPtr<ID3D12Resource> constBuff = nullptr;

        GPUParam* constBuffMapData = nullptr;


        uint32_t cbvIndex = UINT32_MAX;

    }gpuResource{};

    void UploadConstBuff()
    {
        gpuParam.transform.world =
            DirectX::XMMatrixTranspose(cpuParam.world);

        *gpuResource.constBuffMapData = gpuParam;
    }

public:
	static std::shared_ptr<VFXInstance> Create(VFXAsset& asset, Dx12Wrapper& dx12, GpuResourceManager& gpuResMgr);
	HRESULT CreateGpuResource(VFXAsset& asset, Dx12Wrapper& dx12, GpuResourceManager& gpuResMgr);
};