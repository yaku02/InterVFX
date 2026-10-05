#pragma once
#include "Geometry.h"
#include "GraphicsPipeline.h"
#include "IDGenerator.h"
#include "TextureStructs.h"
#include "AssetStructs.h"
#include "ConstantBuffer.h"

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
        GraphicsPipeline::BlendMode blend = GraphicsPipeline::BlendMode::Additive;

        float minLocalAABB[3] = { -50.0f, -20.0f, -50.0f };
        float maxLocalAABB[3] = { 50.0f,  20.0f,  50.0f };
    }cpuParam{};

    struct CBDesc {
        struct Param {
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
        }param;
        
        struct DescIndices {
            float dummy[4];
        }descIndices;

    }cbDesc{};

    struct GPUResource {
        ConstantBuffer<CBDesc> cb;
    }gpuResource{};

    void UploadConstBuff()
    {
        gpuResource.cb.Upload(cbDesc);
    }

    const uint32_t GetCBVIndex() const { return gpuResource.cb.descriptorIndex; }


public:
    static std::shared_ptr<VFXAsset> Create(ID3D12Device* dev, GpuResourceManager& gpuResMgr);
    HRESULT CreateGpuResource(GpuResourceManager& gpuResMgr);
};