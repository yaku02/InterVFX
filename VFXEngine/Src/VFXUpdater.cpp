#include "pch.h"
#include "VFXUpdater.h"
#include "Dx12Wrapper.h"
#include "GpuResourceManager.h"
#include "Debugger.h"
#include "Timer.h"
#include "RootSignatureManager.h"
#include "RootParamLayout.h"

void VFXUpdater::Init(const InitDesc& desc)
{
	m_dx12 = &desc.dx12;
	m_gpuResMgr = &desc.gpuResMgr;

	CreatePipeline(desc.dx12.GetDevice().Get());
}

void VFXUpdater::CreatePipeline(ID3D12Device* dev)
{
	const auto& rootSig = RootSignatureManager::GetRootSignature();

	// --- 共通パラメータの設定 ---
	ComputePipeline::Desc baseDesc;
	baseDesc.csPath = "Engine/Shader/VFXUpdateCS.hlsl";
	baseDesc.csEntry = "CSMain";
	baseDesc.csModel = "cs_6_6";
	baseDesc.rootSignature = rootSig;
	m_updatePipeline = ComputePipeline::Create(dev, baseDesc);
}

void VFXUpdater::Update(const UpdateDesc& desc)
{
    // 1. 安全対策: ポインタが nullptr の場合は何もせず抜ける
    if (!desc.instances || !desc.assetsMap) return;

    auto cmdList = m_dx12->GetCmdList().Get();

    // パーティクル更新
    m_updatePipeline.SetPipeline(cmdList);
    cmdList->SetComputeRoot32BitConstants((UINT)RootParam::Slot::CBVIndices, 1, &desc.globalCBVIndex, (UINT)RootParam::CBVIndices::GlobalCBVIndex);
    cmdList->SetComputeRoot32BitConstants((UINT)RootParam::Slot::CBVIndices, 1, &desc.passCBVIndex, (UINT)RootParam::CBVIndices::PassCBVIndex);

    // 2. ポインタをデリファレンス（*desc.instances）してループ
    for (auto& inst : *desc.instances)
    {
        if (!inst || !inst->cpuParam.isActive) continue;

        // assetsMap ポインタ経由で find 呼び出し
        auto assetIt = desc.assetsMap->find(inst->info.assetID);
        if (assetIt == desc.assetsMap->end())
        {
            Debugger::Log("[Error] VFXUpdater::Update() - Asset not found for instance ID: %u, Asset ID: %u\n", inst->info.base.id, inst->info.assetID);
            continue;
        }
        auto& asset = assetIt->second;

        asset->UploadConstBuff();
        uint32_t assetIndicesCBVIndex = asset->GetCBVIndex();
        cmdList->SetComputeRoot32BitConstants((UINT)RootParam::Slot::CBVIndices, 1, &assetIndicesCBVIndex, (UINT)RootParam::CBVIndices::AssetCBVIndex);

        inst->UploadConstBuff();
        uint32_t instIndicesCBVIndex = inst->GetCBVIndex();
        cmdList->SetComputeRoot32BitConstants((UINT)RootParam::Slot::CBVIndices, 1, &instIndicesCBVIndex, (UINT)RootParam::CBVIndices::InstCBVIndex);

        // 3. Dispatchスレッドグループ数の安全な計算（64の倍数時のオーバーフロー防止）
        uint32_t threadGroupX = (asset->cbDesc.param.numParticles + 63) / 64;
        cmdList->Dispatch(threadGroupX, 1, 1);
    }

    // 全インスタンスをUAVからSRVに状態遷移させる
    std::vector<D3D12_RESOURCE_BARRIER> transitions;
    for (auto& inst : *desc.instances) {
        if (!inst || !inst->cpuParam.isActive) continue;
        auto& instGPU = inst->gpuResource;

        // 4. particleBuffer の存在チェックを追加してクラッシュ防止
        if (instGPU.particleBuffer) {
            transitions.push_back(CD3DX12_RESOURCE_BARRIER::Transition(
                instGPU.particleBuffer.Get(),
                D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
                D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE
            ));
        }
    }

    if (!transitions.empty()) {
        cmdList->ResourceBarrier((UINT)transitions.size(), transitions.data());
    }
}