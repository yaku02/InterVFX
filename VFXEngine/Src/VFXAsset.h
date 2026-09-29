#pragma once
#include "Geometry.h"
#include "PipelineStructs.h"
#include "IDGenerator.h"
#include "TextureStructs.h"
#include "AssetStructs.h"

class Dx12Wrapper;
class GpuResourceManager;

class VFXAsset {
public:
    struct Info {
        inline static uint32_t Counter = 0;
        AssetBaseInfo base{};
    }info{};

    struct CPUParam {
        AABB localAABB{};

        // Emitter
        float spawnRate = 100.0f;

        // --- Rendering
        uint32_t textureId = 0;
        BlendMode blend = BlendMode::Additive;

        float minLocalAABB[3] = { -50.0f, -20.0f, -50.0f };
        float maxLocalAABB[3] = { 50.0f,  20.0f,  50.0f };
    }cpuParam{};

    struct GPUParam {
        // --- Size ---
        float startSize = 1.0f;
        float endSize = 0.5f;

        // --- Behavior ---
        float gravity = -0.002f;
        float drag = 0.1f;

        // --- Color ---
        float startColor[4] = { 0.0f, 1.0f, 0.0f, 1.0f };
        float endColor[4] = { 1.0f, 1.0f, 1.0f, 0.5f };

        float emissive = 1.0f;
        float lifeTime = 5.0f;

        // --- Noise ---
        float noiseStrength = 0.2f;

        uint32_t numParticles = 10000;

    }gpuParam{};

    struct GPUResource {
        Microsoft::WRL::ComPtr<ID3D12Resource> constBuff = nullptr;
        GPUParam* mapData = nullptr;
        uint32_t cbvIndex = 0;

    }gpuResource{};

    void UploadConstBuff()
    {
        *gpuResource.mapData = gpuParam;
    }

public:
    static std::shared_ptr<VFXAsset> Create(ID3D12Device* dev, GpuResourceManager& gpuResMgr);
    HRESULT CreateGpuResource(ID3D12Device* dev, GpuResourceManager& gpuResMgr);
};