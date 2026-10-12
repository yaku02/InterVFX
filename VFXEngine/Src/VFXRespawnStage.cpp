#include "pch.h"
#include "VFXRespawnStage.h"
#include "Dx12Wrapper.h"
#include "GpuResourceManager.h"
#include "Debugger.h"
#include "Timer.h"
#include "RootSignatureManager.h"
#include "RootParamLayout.h"
#include "VFXAsset.h"
#include "VFXInstance.h"
#include "VFXPreview.h"

void VFXRespawnStage::Init(const InitDesc& desc)
{
    m_dx12 = &desc.dx12;
    m_gpuResMgr = &desc.gpuResMgr;

    CreatePipeline(desc.dx12.GetDevice().Get());
}

void VFXRespawnStage::CreatePipeline(ID3D12Device* dev)
{
    const auto& rootSig = RootSignatureManager::GetRootSignature();

    {
        ComputePipeline::Desc desc;
        desc.csPath = "Engine/Shader/VFXRespawnCS.hlsl";
        desc.csEntry = "CSMain";
        desc.csModel = "cs_6_6";
        desc.rootSignature = rootSig;
        m_respawnPipeline = ComputePipeline::Create(dev, desc);
    }
}

void VFXRespawnStage::Respawn(const RespawnDesc& desc)
{
    // 1. 安全対策: 通常インスタンス群またはプレビューのどちらかは存在する必要がある
    bool hasInstances = (desc.instances != nullptr && desc.assetsMap != nullptr);
    bool hasPreview = (desc.preview != nullptr && desc.preview->IsValid());

    if (!hasInstances && !hasPreview) return;

    auto cmdList = m_dx12->GetCmdList().Get();

    m_respawnPipeline.SetPipeline(cmdList);
    cmdList->SetComputeRoot32BitConstants((UINT)RootParam::Slot::CBVIndices, 1, &desc.globalCBVIndex, (UINT)RootParam::CBVIndices::GlobalCBVIndex);
    cmdList->SetComputeRoot32BitConstants((UINT)RootParam::Slot::CBVIndices, 1, &desc.passCBVIndex, (UINT)RootParam::CBVIndices::PassCBVIndex);

    auto RespawnSingleInstance = [&](VFXInstance* inst, VFXAsset* asset) {
        if (!inst || !asset) return;

        // 1. UNORDERED_ACCESS から COPY_DEST へ遷移
        m_gpuResMgr->Transition(cmdList, inst->gpuResource.activeCounterBuffer.Get(), ResState::UnorderedAccess, ResState::CopyDest);

        // 2. 0 が入ったアップロードバッファから値をコピーして 0 クリア
        cmdList->CopyBufferRegion(
            inst->gpuResource.activeCounterBuffer.Get(), 0,
            inst->gpuResource.activeCountUploadBuffer.Get(), 0,
            sizeof(uint32_t)
        );

        // 3. 次の処理（RespawnCS / UpdateCS）のために UNORDERED_ACCESS に戻す
        m_gpuResMgr->Transition(cmdList, inst->gpuResource.activeCounterBuffer.Get(), ResState::CopyDest, ResState::UnorderedAccess);

        asset->UploadConstBuff();
        uint32_t assetIndicesCBVIndex = asset->GetCBVIndex();
        cmdList->SetComputeRoot32BitConstants((UINT)RootParam::Slot::CBVIndices, 1, &assetIndicesCBVIndex, (UINT)RootParam::CBVIndices::AssetCBVIndex);

        inst->UploadConstBuff();
        uint32_t instIndicesCBVIndex = inst->GetCBVIndex();
        cmdList->SetComputeRoot32BitConstants((UINT)RootParam::Slot::CBVIndices, 1, &instIndicesCBVIndex, (UINT)RootParam::CBVIndices::InstCBVIndex);

        uint32_t emitCount = asset->cbDesc.param.emitCount;

        // emitCount に応じたスレッドグループ数を計算（64スレッド/グループ）
        uint32_t threadGroupX = (emitCount + 63) / 64;

        // 0個でなければディスパッチを実行
        if (threadGroupX > 0)
        {
            cmdList->Dispatch(threadGroupX, 1, 1);
        }
        };

    if (hasInstances)
    {
        for (auto& inst : *desc.instances)
        {
            if (!inst || !inst->cpuParam.isActive) continue;

            auto assetIt = desc.assetsMap->find(inst->info.assetID);
            if (assetIt == desc.assetsMap->end())
            {
                Debugger::Log("[Error] VFXRespawnStage::Respawn() - Asset not found for instance ID: %u, Asset ID: %u\n", inst->info.base.id, inst->info.assetID);
                continue;
            }

            RespawnSingleInstance(inst.get(), assetIt->second.get());
        }
    }

    if (hasPreview)
    {
        RespawnSingleInstance(desc.preview->GetInstance(), desc.preview->GetAsset().get());
    }

    // --- C. リソースバリア（UAV -> NON_PIXEL_SHADER_RESOURCE）の適用 ---
    std::vector<D3D12_RESOURCE_BARRIER> transitions;

    // 通常インスタンスのバリア追加
    if (hasInstances)
    {
        for (auto& inst : *desc.instances) {
            if (!inst || !inst->cpuParam.isActive) continue;
            auto& instGPU = inst->gpuResource;

            if (instGPU.deadListBuffer) {
                transitions.push_back(CD3DX12_RESOURCE_BARRIER::UAV(instGPU.deadListBuffer.Get()));
            }
            if (instGPU.deadCounterBuffer) {
                transitions.push_back(CD3DX12_RESOURCE_BARRIER::UAV(instGPU.deadCounterBuffer.Get()));
            }
        }
    }

    // プレビュー用インスタンスのバリア追加
    if (hasPreview)
    {
        auto prevInst = desc.preview->GetInstance();
        if (prevInst && prevInst->gpuResource.deadListBuffer) {
            transitions.push_back(CD3DX12_RESOURCE_BARRIER::UAV(prevInst->gpuResource.deadListBuffer.Get()));
        }
        if (prevInst && prevInst->gpuResource.deadCounterBuffer) {
            transitions.push_back(CD3DX12_RESOURCE_BARRIER::UAV(prevInst->gpuResource.deadCounterBuffer.Get()));
        }
    }

    if (!transitions.empty()) {
        cmdList->ResourceBarrier((UINT)transitions.size(), transitions.data());
    }
}