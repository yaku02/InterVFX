#include "pch.h"
#include "VFXPreview.h"
#include "VFXAsset.h"
#include "VFXInstance.h"

VFXPreview::VFXPreview(
    std::shared_ptr<VFXAsset> asset,
    ID3D12Device* dev,
    ID3D12GraphicsCommandList* cmdList,
    GpuResourceManager& gpuResMgr)
{
    m_targetAsset = asset;
    if (m_targetAsset) {
        m_targetInst = VFXInstance::Create(*m_targetAsset, dev, cmdList, gpuResMgr);
    }

    // アセットが存在し、かつインスタンスの生成にも成功しているか
    m_isValid = (m_targetAsset != nullptr) && (m_targetInst != nullptr);
}