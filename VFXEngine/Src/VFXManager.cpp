#include "pch.h"
#include "Window.h"
#include "Dx12Wrapper.h"
#include "VFXManager.h"
#include "VFXStructs.h"
#include "VFXAsset.h"
#include "Debugger.h"
#include "TextureManager.h"
#include "AssetUI.h"
#include "VFXAssetEditor.h"
#include "VFXInstEditor.h"
#include "InstUI.h"
#include "VFXPass.h"
#include "VFXPreview.h"

VFXManager::VFXManager() = default;
VFXManager::~VFXManager() = default;
void VFXManager::Init(InitDesc& desc)
{
    m_window = &desc.window;
    m_dx12 = &desc.dx12;
    m_gpuResMgr = &desc.gpuResMgr;
    m_texMgr = &desc.texMgr;

    m_pass = std::make_unique<VFXPass>();
    m_pass->Init({ .dx12 = desc.dx12, .gpuResMgr = desc.gpuResMgr });

    m_assetEditor = std::make_unique<VFXAssetEditor>();
    m_instEditor = std::make_unique<VFXInstEditor>();

    m_defaultAssetIcon = m_texMgr->LoadPNG("Engine/Icons/VFX_Icon2.png");

    AssetUI::RegisterCreationCallback(
        AssetType::VFX,
        [this]() -> HRESULT {
            return this->CreateAndRegisterAsset();
        }
    );

    AssetUI::RegisterSelectCallback(
        AssetType::VFX,
        [this](uint32_t id) -> void {
            return this->OnAssetSelected(id);
        }
    );

    AssetUI::RegisterDeleteCallback(
        AssetType::VFX,
        [this](uint32_t id) -> void {
            return this->OnDeleteAssetSelected(id);
        }
    );

    InstUI::RegisterDnDCreateCallback(
        InstType::VFX,
        [this](uint32_t id) -> HRESULT {
            return this->CreateAndRegisterInstance(id);
        }
    );

    InstUI::RegisterSelectCallback(
        InstType::VFX,
        [this](uint32_t id) ->void {
            return this->OnInstSelected(id);
        }
    );

    InstUI::RegisterDeleteCallback(
        InstType::VFX,
        [this](uint32_t id) ->void {
            return this->OnDeleteInstSelected(id);
        }
    );

    VFXAssetEditor::RegisterChangeMaxParticleCallback(
        [this](uint32_t id, uint32_t afterMaxParticles) -> void {
            return this->OnMaxParticleChanged(id, afterMaxParticles);
        }
    );
}

VFXAsset* VFXManager::GetAsset(const uint32_t assetID)
{
    auto it = m_assetsMap.find(assetID);
    if (it == m_assetsMap.end()) {
        Debugger::Log("VFX asset not found: %u\n", assetID);
        return nullptr;
    }
    return it->second.get(); // 生ポインタを返す（参照カウンタは増えない）
}

VFXInstance* VFXManager::GetInst(const uint32_t instID)
{
    auto it = m_instsMap.find(instID);
    if (it == m_instsMap.end()) {
        Debugger::Log("VFX instance not found: %u\n", instID);
        return nullptr;
    }
    return it->second.get();
}

HRESULT VFXManager::CreateAndRegisterAsset()
{
    auto asset = VFXAsset::Create(m_dx12->GetDevice().Get(), *m_gpuResMgr);

    if (!asset)
    {
        Debugger::Log("Create failed");
        return E_FAIL;
    }
    asset->info.base.icon = m_defaultAssetIcon;

    AssetUI::Add(asset->info.base);
    m_assets.push_back(asset);
    m_assetsMap[asset->info.base.id] = asset;

    return S_OK;
}

HRESULT VFXManager::CreateAndRegisterInstance(const uint32_t assetID)
{
    ID3D12Device* dev = m_dx12->GetDevice().Get();
    ID3D12GraphicsCommandList* cmdList = m_dx12->GetCmdList().Get();

    VFXAsset* asset = GetAsset(assetID);

    auto inst = VFXInstance::Create(*asset, dev, cmdList, *m_gpuResMgr);
    if (!inst) return E_FAIL;
    InstUI::Add(inst->info.base);

    m_insts.push_back(inst);
    Debugger::Log("Created VFX instance: %s\n", inst->info.base.name.c_str());
    m_instsMap[inst->info.base.id] = inst;

    return S_OK;
}

void VFXManager::OnDeleteAssetSelected(const uint32_t id)
{
    auto it = m_assetsMap.find(id);
    if (it == m_assetsMap.end()) return;
    m_pendingDeleteAsset = it->second;
}

bool VFXManager::DeleteAsset(std::shared_ptr<VFXAsset> target)
{
    // 1. targetが有効かチェック
    if (!target) return false;

    m_dx12->FlushCommandQueue();

    uint32_t assetID = target->info.base.id;

    // --- A. このアセットから生成されたインスタンスを全て特定して削除する ---
    // 1. m_instsMap から該当インスタンスのエントリを削除し、プレビューやエディタの選択も解除
    for (auto it = m_instsMap.begin(); it != m_instsMap.end(); )
    {
        if (it->second && it->second->info.assetID == assetID)
        {
            // エディタやプレビューのターゲットになっていれば解除
            if (m_instEditor)
            {
                m_instEditor->SetTarget(nullptr);
            }
            it = m_instsMap.erase(it);
        }
        else
        {
            ++it;
        }
    }

    // 2. m_insts (vector) から該当インスタンスを一括削除
    m_insts.erase(
        std::remove_if(m_insts.begin(), m_insts.end(), [assetID](const auto& inst) {
            return inst && inst->info.assetID == assetID;
            }),
        m_insts.end()
    );

    // 3. もし現在プレビューで表示しているインスタンスが、この削除アセットのものだったらプレビューも破棄
    if (m_preview && m_preview->GetAsset() == target) {
        m_preview = nullptr;
    }

    // --- B. アセット自体の削除処理 ---
    // 1. unordered_map (m_assetsMap) から削除
    m_assetsMap.erase(assetID);

    // 2. std::vector (m_assets) から削除
    m_assets.erase(
        std::remove_if(m_assets.begin(), m_assets.end(), [assetID](const auto& asset) {
            return asset && asset->info.base.id == assetID;
            }),
        m_assets.end()
    );

    // 3. エディタのターゲット解除
    if (m_assetEditor)
    {
        m_assetEditor->SetTarget(nullptr);
    }

    // 4. プレビュー用アセットの予約クリア
    if (m_pendingPreviewAsset == target)
    {
        m_pendingPreviewAsset = nullptr;
    }

    Debugger::Log("Delete VFX Asset and its instances: %s\n", target->info.base.name.c_str());

    return true;
}

void VFXManager::OpenAssetEditor(const uint32_t assetID)
{
    auto* asset = GetAsset(assetID);
    if (!asset) return;
    m_assetEditor->Open(*asset);
}

void VFXManager::OnAssetSelected(const uint32_t assetID)
{
    // m_assetsMap から shared_ptr を直接探す
    auto it = m_assetsMap.find(assetID);
    if (it == m_assetsMap.end()) {
        Debugger::Log("VFX asset not found: %u\n", assetID);
        return;
    }

    std::shared_ptr<VFXAsset> asset = it->second; // shared_ptr を取得

    // エディタへは生ポインタを渡す (.get())
    if (m_assetEditor) {
        m_assetEditor->SetTarget(asset.get());
    }

    // プレビューを作る準備をする
    m_pendingPreviewAsset = asset;
}

void VFXManager::OpenInstEditor(const uint32_t instID)
{
    auto* inst = GetInst(instID);
    if (!inst) return;
    m_instEditor->Open(*inst);
}

void VFXManager::OnInstSelected(const uint32_t instID)
{
    VFXInstance* inst = GetInst(instID);
    if (!inst) return;

    // VFXManager が仲介して各コンポーネントへ渡す
    if (m_instEditor) {
        m_instEditor->SetTarget(inst);
    }
}

void VFXManager::OnDeleteInstSelected(const uint32_t id)
{
    auto it = m_instsMap.find(id);
    if (it == m_instsMap.end()) return;
    m_pendingDeleteInst = it->second;
}

bool VFXManager::DeleteInst(std::shared_ptr<VFXInstance> target)
{
    // 1. targetが有効かチェック
    if (!target) return false;

    m_dx12->FlushCommandQueue();

    uint32_t instID = target->info.base.id;

    m_instsMap.erase(instID);

    m_insts.erase(
        std::remove_if(m_insts.begin(), m_insts.end(), [instID](const auto& inst) {
            return inst && inst->info.base.id == instID;
            }),
        m_insts.end()
    );

    if (m_instEditor)
    {
        m_instEditor->SetTarget(nullptr);
    }

    if (m_preview && m_preview->GetInstance()->info.base.id == target->info.base.id) {
        m_preview = nullptr;
    }

    Debugger::Log("Delete VFX Instance: %s", target->info.assetName.c_str());

    return true;
}

bool VFXManager::InitDeadListAndCounter()
{
    m_dx12->FlushCommandQueue();

    ID3D12Device* dev = m_dx12->GetDevice().Get();
    ID3D12GraphicsCommandList* cmdList = m_dx12->GetCmdList().Get();

    for (auto& inst : m_insts) {
        if (inst && inst->info.assetID == m_pendingChangeMaxParticlesAssetID) {
            // デッドリスト（およびカウンター）バッファの再構築・初期化を呼び出す
            inst->CreateDeadListBuffer(dev, cmdList, *m_gpuResMgr, m_pendingAfterMaxParticles);
        }
    }

    if (m_preview) {
        if (m_preview->GetInstance()->info.assetID == m_pendingChangeMaxParticlesAssetID) {
            m_preview->GetInstance()->CreateDeadListBuffer(dev, cmdList, *m_gpuResMgr, m_pendingAfterMaxParticles);
        }
    }

    m_pendingChangeMaxParticlesAssetID = UINT32_MAX;
    m_pendingAfterMaxParticles = UINT32_MAX;

    Debugger::Log("Init DeadList and Counter\n");
    return true;
}

void VFXManager::ShutDown()
{
    m_defaultAssetIcon.buffer.Reset();
    m_assets.clear();
    m_insts.clear();
}

void VFXManager::Execute(const ExecuteDesc& desc)
{
    if (m_pendingPreviewAsset)
    {
        // ★ 1. 古いプレビューを破棄する前に、GPU がこれまでのコマンドを処理し終えるのを完全に待つ
        if (m_dx12 && m_preview)
        {
            m_dx12->FlushCommandQueue();
        }

        // ★ 2. 安全に古いプレビューを破棄
        m_preview.reset();

        ID3D12Device* dev = m_dx12->GetDevice().Get();
        ID3D12GraphicsCommandList* cmdList = m_dx12->GetCmdList().Get();

        // 新しいプレビューを生成
        m_preview = std::make_unique<VFXPreview>(m_pendingPreviewAsset, dev, cmdList, *m_gpuResMgr);
        if (!m_preview->IsValid()) {
            Debugger::Log("[Error] Failed to create VFXPreview\n");
            m_preview.reset();
        }

        m_pendingPreviewAsset = nullptr; // 予約クリア
    }

    if (m_pendingDeleteAsset) {
        DeleteAsset(m_pendingDeleteAsset);
        m_pendingDeleteAsset = nullptr;
    }

    if (m_pendingDeleteInst) {
        DeleteInst(m_pendingDeleteInst);
        m_pendingDeleteInst = nullptr;
    }

    
    if (m_pendingChangeMaxParticlesAssetID != UINT32_MAX && m_pendingAfterMaxParticles != UINT32_MAX) {
        InitDeadListAndCounter();
    }

    if (!m_pass) return;

    VFXPass::PassExecuteDesc passDesc{};
    passDesc.updateDesc.globalCBVIndex = desc.grobalCBVIndex;
    passDesc.renderDesc.globalCBVIndex = desc.grobalCBVIndex;

    if (desc.mode == RenderMode::Scene)
    {
        // --- シーン描画モード ---
        passDesc.updateDesc.instances = &m_insts;
        passDesc.updateDesc.assetsMap = &m_assetsMap;
        passDesc.updateDesc.preview = nullptr;

        passDesc.renderDesc.instances = &m_insts;
        passDesc.renderDesc.assetsMap = &m_assetsMap;
        passDesc.renderDesc.preview = nullptr;

        passDesc.respawnDesc.instances = &m_insts;
        passDesc.respawnDesc.assetsMap = &m_assetsMap;
        passDesc.respawnDesc.preview = nullptr;

        passDesc.copyArgsDesc.instances = &m_insts;
        passDesc.copyArgsDesc.assetsMap = &m_assetsMap;
        passDesc.copyArgsDesc.preview = nullptr;

    }
    else if (desc.mode == RenderMode::PreviewOnly)
    {
        // --- プレビュー描画モード ---
        // シーン上のインスタンスは描画せず、m_preview のみ渡す
        passDesc.updateDesc.instances = nullptr;
        passDesc.updateDesc.assetsMap = nullptr;
        passDesc.updateDesc.preview = m_preview.get();

        passDesc.renderDesc.instances = nullptr;
        passDesc.renderDesc.assetsMap = nullptr;
        passDesc.renderDesc.preview = m_preview.get();

        passDesc.respawnDesc.instances = nullptr;
        passDesc.respawnDesc.assetsMap = nullptr;
        passDesc.respawnDesc.preview = m_preview.get();;

        passDesc.copyArgsDesc.instances = nullptr;
        passDesc.copyArgsDesc.assetsMap = nullptr;
        passDesc.copyArgsDesc.preview = m_preview.get();;
    }

    // VFXPass (Updater -> Renderer) の実行
    m_pass->Execute(passDesc);
}