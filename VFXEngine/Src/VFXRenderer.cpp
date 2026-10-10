#include "pch.h"
#include "VFXRenderer.h"
#include "Dx12Wrapper.h"
#include "GpuResourceManager.h"
#include "RootSignatureManager.h"
#include "VFXInstance.h"
#include "VFXAsset.h"
#include "RootParamLayout.h"
#include "VFXPreview.h"

void VFXRenderer::Init(const InitDesc& desc)
{
	m_dx12 = &desc.dx12;
	m_gpuResMgr = &desc.gpuResMgr;
	CreatePipeline(desc.dx12.GetDevice().Get());
}

void VFXRenderer::CreatePipeline(ID3D12Device* dev)
{
    const auto& rootSig = RootSignatureManager::GetRootSignature();

    // VSとPSを作成
            // --- 共通パラメータの設定 ---
    GraphicsPipeline::Desc baseDesc;
    baseDesc.vsPath = "Engine/Shader/VFXVS.hlsl";
    baseDesc.psPath = "Engine/Shader/VFXPS.hlsl";
    baseDesc.vsEntry = "VSMain";
    baseDesc.psEntry = "PSMain";
    baseDesc.vsModel = "vs_6_6";
    baseDesc.psModel = "ps_6_6";
    baseDesc.rootSignature = rootSig;

    // ネイティブ構造体（d3dDesc）側の共通描画ステートを設定
    baseDesc.d3dDesc.RTVFormats[0] = DXGI_FORMAT_R16G16B16A16_FLOAT;
    baseDesc.d3dDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
    baseDesc.d3dDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

    // VFX用共通ステート: 深度テストは行うが書き込みは行わない（Z-Write OFF）
    baseDesc.d3dDesc.DepthStencilState.DepthEnable = TRUE;
    baseDesc.d3dDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
    baseDesc.d3dDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE; // カリングなし（両面描画）

    // --------------------------------------------------
    // 1. 加算合成 (Additive) PSO
    // --------------------------------------------------
    GraphicsPipeline::Desc additiveDesc = baseDesc;
    additiveDesc.blendMode = GraphicsPipeline::BlendMode::Additive;

    m_additivePipeline = GraphicsPipeline::Create(dev, additiveDesc);

    // --------------------------------------------------
    // 2. アルファ半透明 (Alpha) PSO
    // --------------------------------------------------
    GraphicsPipeline::Desc alphaDesc = baseDesc;
    alphaDesc.blendMode = GraphicsPipeline::BlendMode::Alpha;

    m_alphaPipeline = GraphicsPipeline::Create(dev, alphaDesc);
}

void VFXRenderer::Render(const RenderDesc& desc)
{
    // 1. 安全対策: 通常インスタンス群またはプレビューのどちらかは存在する必要がある
    bool hasInstances = (desc.instances != nullptr && desc.assetsMap != nullptr);
    bool hasPreview = (desc.preview != nullptr && desc.preview->IsValid());

    if (!hasInstances && !hasPreview) return;

    auto cmdList = m_dx12->GetCmdList().Get();

    // パーティクル描画用パイプラインと定数の共通セット
    m_additivePipeline.SetPipeline(cmdList);
    cmdList->SetGraphicsRoot32BitConstants((UINT)RootParam::Slot::CBVIndices, 1, &desc.globalCBVIndex, (UINT)RootParam::CBVIndices::GlobalCBVIndex);
    cmdList->SetGraphicsRoot32BitConstants((UINT)RootParam::Slot::CBVIndices, 1, &desc.passCBVIndex, (UINT)RootParam::CBVIndices::PassCBVIndex);

    cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

    // 単一要素を描画するヘルパー（既存のルートパラメータ指定をそのまま使用）
    auto DrawSingleInstance = [&](VFXInstance* inst, VFXAsset* asset) {
        if (!inst || !asset) return;

        asset->UploadConstBuff();
        uint32_t assetIndicesCBVIndex = asset->GetCBVIndex();
        cmdList->SetGraphicsRoot32BitConstants((UINT)RootParam::Slot::CBVIndices, 1, &assetIndicesCBVIndex, (UINT)RootParam::CBVIndices::AssetCBVIndex);

        inst->UploadConstBuff();
        uint32_t instIndicesCBVIndex = inst->GetCBVIndex();
        cmdList->SetGraphicsRoot32BitConstants((UINT)RootParam::Slot::CBVIndices, 1, &instIndicesCBVIndex, (UINT)RootParam::CBVIndices::InstCBVIndex);

        cmdList->DrawInstanced(4, asset->cbDesc.param.numParticles, 0, 0);
        };

    // --- A. 通常インスタンス群の描画 ---
    if (hasInstances)
    {
        for (auto& inst : *desc.instances)
        {
            if (!inst || !inst->cpuParam.isActive) continue;

            auto assetIt = desc.assetsMap->find(inst->info.assetID);
            if (assetIt == desc.assetsMap->end())
            {
                Debugger::Log("[Error] VFXRenderer::Render() - Asset not found for instance ID: %u, Asset ID: %u\n", inst->info.base.id, inst->info.assetID);
                continue;
            }

            DrawSingleInstance(inst.get(), assetIt->second.get());
        }
    }

    // --- B. プレビュー用インスタンスの描画 ---
    if (hasPreview)
    {
        DrawSingleInstance(desc.preview->GetInstance(), desc.preview->GetAsset().get());
    }

    // --- C. リソースバリア（SRV -> UAV）の適用（次フレームのUpdateCSに備える） ---
    std::vector<D3D12_RESOURCE_BARRIER> transitions;

    // 通常インスタンスのバリア追加
    if (hasInstances)
    {
        for (auto& inst : *desc.instances) {
            if (!inst || !inst->cpuParam.isActive) continue;
            auto& instGPU = inst->gpuResource;

            if (instGPU.particleBuffer) {
                transitions.push_back(CD3DX12_RESOURCE_BARRIER::Transition(
                    instGPU.particleBuffer.Get(),
                    D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
                    D3D12_RESOURCE_STATE_UNORDERED_ACCESS
                ));
            }
        }
    }

    // プレビュー用インスタンスのバリア追加
    if (hasPreview)
    {
        VFXInstance* prevInst = desc.preview->GetInstance();
        if (prevInst && prevInst->gpuResource.particleBuffer) {
            transitions.push_back(CD3DX12_RESOURCE_BARRIER::Transition(
                prevInst->gpuResource.particleBuffer.Get(),
                D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
                D3D12_RESOURCE_STATE_UNORDERED_ACCESS
            ));
        }
    }

    if (!transitions.empty()) {
        cmdList->ResourceBarrier((UINT)transitions.size(), transitions.data());
    }
}