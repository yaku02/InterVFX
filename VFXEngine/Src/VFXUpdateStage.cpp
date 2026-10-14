#include "pch.h"
#include "VFXUpdateStage.h"
#include "Dx12Wrapper.h"
#include "GpuResourceManager.h"
#include "Debugger.h"
#include "Timer.h"
#include "RootSignatureManager.h"
#include "RootParamLayout.h"
#include "VFXAsset.h"
#include "VFXInstance.h"
#include "VFXPreview.h"

void VFXUpdateStage::Init(const InitDesc& desc)
{
	m_dx12 = &desc.dx12;
	m_gpuResMgr = &desc.gpuResMgr;

	CreatePipeline(desc.dx12.GetDevice().Get());

}

void VFXUpdateStage::CreatePipeline(ID3D12Device* dev)
{
	const auto& rootSig = RootSignatureManager::GetRootSignature();
    
    {
        ComputePipeline::Desc desc;
        desc.csPath = "Engine/Shader/VFXUpdateCS.hlsl";
        desc.csEntry = "CSMain";
        desc.csModel = "cs_6_6";
        desc.rootSignature = rootSig;
        m_updatePipeline = ComputePipeline::Create(dev, desc);
    }
}

void VFXUpdateStage::Update(const UpdateDesc& desc)
{
    // 1. 安全対策: 通常インスタンス群またはプレビューのどちらかは存在する必要がある
    bool hasInstances = (desc.instances != nullptr && desc.assetsMap != nullptr);
    bool hasPreview = (desc.preview != nullptr && desc.preview->IsValid());

    if (!hasInstances && !hasPreview) return;

    auto cmdList = m_dx12->GetCmdList().Get();

    // パーティクル更新
    m_updatePipeline.SetPipeline(cmdList);
    cmdList->SetComputeRoot32BitConstants((UINT)RootParam::Slot::CBVIndices, 1, &desc.globalCBVIndex, (UINT)RootParam::CBVIndices::GlobalCBVIndex);
    cmdList->SetComputeRoot32BitConstants((UINT)RootParam::Slot::CBVIndices, 1, &desc.passCBVIndex, (UINT)RootParam::CBVIndices::PassCBVIndex);

    // 単一要素の更新用ヘルパー（既存のルートパラメータ指定をそのまま使用）
    auto UpdateSingleInstance = [&](VFXInstance* inst, VFXAsset* asset) {
        if (!inst || !asset) return;

        asset->UploadConstBuff();
        uint32_t assetIndicesCBVIndex = asset->GetCBVIndex();
        cmdList->SetComputeRoot32BitConstants((UINT)RootParam::Slot::CBVIndices, 1, &assetIndicesCBVIndex, (UINT)RootParam::CBVIndices::AssetCBVIndex);

        inst->UploadConstBuff();
        uint32_t instIndicesCBVIndex = inst->GetCBVIndex();
        cmdList->SetComputeRoot32BitConstants((UINT)RootParam::Slot::CBVIndices, 1, &instIndicesCBVIndex, (UINT)RootParam::CBVIndices::InstCBVIndex);

        uint32_t threadGroupX = (asset->cbDesc.param.maxParticles + 63) / 64;
        cmdList->Dispatch(threadGroupX, 1, 1);
        };

    // --- A. 通常インスタンス群の更新 ---
    if (hasInstances)
    {
        for (auto& inst : *desc.instances)
        {
            if (!inst || !inst->cpuParam.isActive) continue;

            auto assetIt = desc.assetsMap->find(inst->info.assetID);
            if (assetIt == desc.assetsMap->end())
            {
                Debugger::Log("[Error] VFXUpdateStage::Update() - Asset not found for instance ID: %u, Asset ID: %u\n", inst->info.base.id, inst->info.assetID);
                continue;
            }

            UpdateSingleInstance(inst.get(), assetIt->second.get());
        }
    }

    // --- B. プレビュー用インスタンスの更新 ---
    if (hasPreview)
    {
        UpdateSingleInstance(desc.preview->GetInstance(), desc.preview->GetAsset().get());
    }

    // --- C. リソースバリア（UAV -> NON_PIXEL_SHADER_RESOURCE）の適用 ---
    std::vector<D3D12_RESOURCE_BARRIER> transitions;

    // 通常インスタンスのバリア追加
    if (hasInstances)
    {
        for (auto& inst : *desc.instances) {
            if (!inst || !inst->cpuParam.isActive) continue;
            auto& instGPU = inst->gpuResource;

            if (instGPU.particleBuffer) {
                transitions.push_back(m_gpuResMgr->Transition(instGPU.particleBuffer.Get(), ResState::UnorderedAccess, ResState::NonPixelShaderResource));
            }
        }
    }

    // プレビュー用インスタンスのバリア追加
    if (hasPreview)
    {
        VFXInstance* prevInst = desc.preview->GetInstance();
        auto& instGPU = prevInst->gpuResource;
        if (prevInst && instGPU.particleBuffer) {
            transitions.push_back(m_gpuResMgr->Transition(instGPU.particleBuffer.Get(), ResState::UnorderedAccess, ResState::NonPixelShaderResource));
        }
    }

    if (!transitions.empty()) {
        cmdList->ResourceBarrier((UINT)transitions.size(), transitions.data());
    }
}