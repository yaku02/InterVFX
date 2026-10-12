#include "pch.h"
#include "VFXCopyArgsStage.h"
#include "Dx12Wrapper.h"
#include "GpuResourceManager.h"
#include "Debugger.h"
#include "RootSignatureManager.h"
#include "RootParamLayout.h"
#include "VFXAsset.h"
#include "VFXInstance.h"
#include "VFXPreview.h"

void VFXCopyArgsStage::Init(const InitDesc& desc)
{
    m_dx12 = &desc.dx12;
    m_gpuResMgr = &desc.gpuResMgr;

    CreatePipeline(desc.dx12.GetDevice().Get());
}

void VFXCopyArgsStage::CreatePipeline(ID3D12Device* dev)
{
    const auto& rootSig = RootSignatureManager::GetRootSignature();

    {
        ComputePipeline::Desc desc;
        desc.csPath = "Engine/Shader/VFXCopyArgsCS.hlsl"; // カウンターを引数バッファへコピーするシェーダ-
        desc.csEntry = "CSMain";
        desc.csModel = "cs_6_6";
        desc.rootSignature = rootSig;
        m_copyArgsPipeline = ComputePipeline::Create(dev, desc);
    }
}

void VFXCopyArgsStage::CopyArgs(const ExeDesc& desc)
{
    // 1. 安全対策: 通常インスタンス群またはプレビューのどちらかは存在する必要がある
    bool hasInstances = (desc.instances != nullptr && desc.assetsMap != nullptr);
    bool hasPreview = (desc.preview != nullptr && desc.preview->IsValid());

    if (!hasInstances && !hasPreview) return;

    auto cmdList = m_dx12->GetCmdList().Get();

    // カウンターコピー用パイプラインと定数の共通セット
    m_copyArgsPipeline.SetPipeline(cmdList);
    cmdList->SetComputeRoot32BitConstants((UINT)RootParam::Slot::CBVIndices, 1, &desc.globalCBVIndex, (UINT)RootParam::CBVIndices::GlobalCBVIndex);
    cmdList->SetComputeRoot32BitConstants((UINT)RootParam::Slot::CBVIndices, 1, &desc.passCBVIndex, (UINT)RootParam::CBVIndices::PassCBVIndex);

    // 単一要素のコピー処理を実行するヘルパー
    auto CopySingleInstanceArgs = [&](VFXInstance* inst, VFXAsset* asset) {
        if (!inst || !asset) return;

        // アセットの CBV 設定
        asset->UploadConstBuff();
        uint32_t assetIndicesCBVIndex = asset->GetCBVIndex();
        cmdList->SetComputeRoot32BitConstants((UINT)RootParam::Slot::CBVIndices, 1, &assetIndicesCBVIndex, (UINT)RootParam::CBVIndices::AssetCBVIndex);

        // インスタンスの CBV 設定（activeCounterBuffer や indirectArgsUAV のインデックスを含む）
        inst->UploadConstBuff();
        uint32_t instIndicesCBVIndex = inst->GetCBVIndex();
        cmdList->SetComputeRoot32BitConstants((UINT)RootParam::Slot::CBVIndices, 1, &instIndicesCBVIndex, (UINT)RootParam::CBVIndices::InstCBVIndex);

        // ★ UpdateCS で書き込んだ activeCounterBuffer を CopyArgsCS で安全に読むための UAV バリア
        CD3DX12_RESOURCE_BARRIER uavBarrier = CD3DX12_RESOURCE_BARRIER::UAV(inst->gpuResource.activeCounterBuffer.Get());
        cmdList->ResourceBarrier(1, &uavBarrier);

        // カウンターの値（1つの数値）をコピーするだけなので 1x1x1 で Dispatch
        cmdList->Dispatch(1, 1, 1);
        };

    // --- A. 通常インスタンス群のコピー処理 ---
    if (hasInstances)
    {
        for (auto& inst : *desc.instances)
        {
            if (!inst || !inst->cpuParam.isActive) continue;

            auto assetIt = desc.assetsMap->find(inst->info.assetID);
            if (assetIt == desc.assetsMap->end())
            {
                Debugger::Log("[Error] VFXCopyArgsStage::CopyArgs() - Asset not found for instance ID: %u, Asset ID: %u\n", inst->info.base.id, inst->info.assetID);
                continue;
            }

            CopySingleInstanceArgs(inst.get(), assetIt->second.get());
        }
    }

    // --- B. プレビュー用インスタンスのコピー処理 ---
    if (hasPreview)
    {
        CopySingleInstanceArgs(desc.preview->GetInstance(), desc.preview->GetAsset().get());
    }

    // --- C. リソースバリア（IndirectArgsBuffer を描画時の INDIRECT_ARGUMENT ステートへ遷移する準備） ---
    std::vector<D3D12_RESOURCE_BARRIER> transitions;

    if (hasInstances)
    {
        for (auto& inst : *desc.instances) {
            if (!inst || !inst->cpuParam.isActive) continue;
            auto& instGPU = inst->gpuResource;

            if (instGPU.indirectArgsBuffer) {
                transitions.push_back(
                    m_gpuResMgr->Transition(instGPU.indirectArgsBuffer.Get(), ResState::UnorderedAccess, ResState::IndirectArgument)
                );
            }
        }
    }

    if (hasPreview)
    {
        auto prevInst = desc.preview->GetInstance();
        auto& instGPU = prevInst->gpuResource;

        if (instGPU.indirectArgsBuffer) {
            transitions.push_back(
                m_gpuResMgr->Transition(instGPU.indirectArgsBuffer.Get(), ResState::UnorderedAccess, ResState::IndirectArgument)
            );
        }
    }

    if (!transitions.empty()) {
        cmdList->ResourceBarrier((UINT)transitions.size(), transitions.data());
    }
}