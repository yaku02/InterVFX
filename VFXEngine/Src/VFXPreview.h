#pragma once
#include "ConstantBuffer.h"

class VFXAsset;
class VFXInstance;
class GpuResourceManager;
class VFXPreview {
public:
    VFXPreview(
        std::shared_ptr<VFXAsset> asset,
        ID3D12Device* dev,
        ID3D12GraphicsCommandList* cmdList,
        GpuResourceManager& gpuResMgr);


    bool IsValid() const { return m_isValid; }

    VFXInstance* GetInstance() const { return m_targetInst.get(); }
    std::shared_ptr<VFXAsset> GetAsset() const { return m_targetAsset; }

private:
    bool m_isValid = false;
    std::shared_ptr<VFXAsset> m_targetAsset = nullptr;
    std::shared_ptr<VFXInstance> m_targetInst = nullptr; // または shared_ptr
};