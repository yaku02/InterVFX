#include "pch.h"
#include "PreviewUI.h"
#include "Window.h"
#include "Dx12Wrapper.h"
#include "GpuResourceManager.h"
#include "Debugger.h"
#include <imgui.h>
#include <d3dx12.h>

void PreviewUI::Init(InitDesc& initDesc)
{
    m_window = &initDesc.window;
    m_dx12 = &initDesc.dx12;
    m_gpuResMgr = &initDesc.gpuResMgr;
}

void PreviewUI::Render()
{
    // 遅延リサイズの実行
    if (m_needResize)
    {
        ResizeBuffers(m_pendingWidth, m_pendingHeight);
        m_needResize = false;
    }

    if (!m_colorBuffer.resource || m_bufferWidth == 0 || m_bufferHeight == 0) return;

    const auto& cmdList = m_dx12->GetCmdList().Get();

    // 1. リソースバリア：SHADER_RESOURCE -> RENDER_TARGET
    D3D12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
        m_colorBuffer.resource.Get(),
        D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
        D3D12_RESOURCE_STATE_RENDER_TARGET
    );
    cmdList->ResourceBarrier(1, &barrier);

    // クリア処理
    float clearColor[] = { 0.118f, 0.565f, 1.0f, 1.0f }; // 背景色（水色系）
    cmdList->ClearRenderTargetView(m_colorBuffer.rtvHandle, clearColor, 0, nullptr);
    cmdList->ClearDepthStencilView(m_depthBuffer.dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

    // RenderTarget と DepthStencil のバインド
    cmdList->OMSetRenderTargets(1, &m_colorBuffer.rtvHandle, FALSE, &m_depthBuffer.dsvHandle);

    // ビューポートとシザー矩形の設定
    D3D12_VIEWPORT viewport = {
        0.0f, 0.0f,
        static_cast<float>(m_bufferWidth),
        static_cast<float>(m_bufferHeight),
        0.0f, 1.0f
    };
    D3D12_RECT scissorRect = {
        0, 0,
        static_cast<LONG>(m_bufferWidth),
        static_cast<LONG>(m_bufferHeight)
    };
    cmdList->RSSetViewports(1, &viewport);
    cmdList->RSSetScissorRects(1, &scissorRect);

    // 2. 登録された描画処理の呼び出し
    if (m_renderCallback) {
        m_renderCallback();
    }

    // 3. リソースバリア：RENDER_TARGET -> SHADER_RESOURCE (ImGui用)
    barrier = CD3DX12_RESOURCE_BARRIER::Transition(
        m_colorBuffer.resource.Get(),
        D3D12_RESOURCE_STATE_RENDER_TARGET,
        D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE
    );
    cmdList->ResourceBarrier(1, &barrier);
}

void PreviewUI::DrawInParentUI()
{
    ImVec2 availSize = ImGui::GetContentRegionAvail();
    if (availSize.x <= 0.0f || availSize.y <= 0.0f) return;

    ImGuiWindowFlags childFlags = ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse;

    if (ImGui::BeginChild("PreviewContentRegion", availSize, false, childFlags))
    {
        // 1. 中央揃えタイトルの描画
        const char* titleText = "PREVIEW";
        float titleWidth = ImGui::CalcTextSize(titleText).x;
        float centeredPosX = (ImGui::GetContentRegionAvail().x - titleWidth) * 0.5f;
        if (centeredPosX > 0.0f)
        {
            ImGui::SetCursorPosX(centeredPosX);
        }
        ImGui::TextColored(ImVec4(1.0f, 1.0f, 1.0f, 1.0f), "%s", titleText);

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        // 2. 残り領域のサイズを計算し、ビューポートテクスチャを描画
        ImVec2 viewportSize = ImGui::GetContentRegionAvail();
        uint32_t newWidth = static_cast<uint32_t>(viewportSize.x);
        uint32_t newHeight = static_cast<uint32_t>(viewportSize.y);

        if (newWidth > 0 && newHeight > 0)
        {
            if (newWidth != m_bufferWidth || newHeight != m_bufferHeight)
            {
                m_needResize = true;
                m_pendingWidth = newWidth;
                m_pendingHeight = newHeight;
            }

            if (m_colorBuffer.srvHandle.ptr != 0)
            {
                ImTextureID textureId = (ImTextureID)m_colorBuffer.srvHandle.ptr;
                ImGui::Image(textureId, viewportSize);
            }
            else
            {
                const char* noTexText = "No Preview Texture Set.";
                float noTexWidth = ImGui::CalcTextSize(noTexText).x;
                float noTexPosX = (viewportSize.x - noTexWidth) * 0.5f;
                if (noTexPosX > 0.0f) ImGui::SetCursorPosX(noTexPosX);

                ImGui::TextDisabled("%s", noTexText);
            }
        }
    }
    ImGui::EndChild();
}

HRESULT PreviewUI::CreateColorBuffer(uint32_t width, uint32_t height)
{
    // ★ ガード処理：サイズが 0、または DX12 テクスチャの上限（16384）を超えている場合は処理しない
    if (width == 0 || height == 0 || width > D3D12_REQ_TEXTURE2D_U_OR_V_DIMENSION || height > D3D12_REQ_TEXTURE2D_U_OR_V_DIMENSION)
    {
        Debugger::Log("[Warning] Invalid dimensions for ColorBuffer: %u x %u\n", width, height);
        return E_INVALIDARG;
    }

    const auto& dev = m_dx12->GetDevice().Get();

    D3D12_HEAP_PROPERTIES heapProp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);

    D3D12_RESOURCE_DESC resDesc = CD3DX12_RESOURCE_DESC::Tex2D(
        DXGI_FORMAT_R16G16B16A16_FLOAT,
        width,
        height,
        1, 1, 1, 0,
        D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET);

    float clearColor[] = { 0.118f, 0.565f, 1.0f, 1.0f };
    D3D12_CLEAR_VALUE cv = CD3DX12_CLEAR_VALUE(DXGI_FORMAT_R16G16B16A16_FLOAT, clearColor);

    dev->CreateCommittedResource(
        &heapProp,
        D3D12_HEAP_FLAG_NONE,
        &resDesc,
        D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
        &cv,
        IID_PPV_ARGS(&m_colorBuffer.resource));

    m_colorBuffer.rtvHandle = m_gpuResMgr->AllocateDescriptor(nullptr, nullptr, HeapType::Rtv);
    dev->CreateRenderTargetView(m_colorBuffer.resource.Get(), nullptr, m_colorBuffer.rtvHandle);

    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.Texture2D.MipLevels = 1;

    D3D12_CPU_DESCRIPTOR_HANDLE srvHandle =
        m_gpuResMgr->AllocateDescriptor(&m_colorBuffer.srvHandle, nullptr, HeapType::ImGui);

    dev->CreateShaderResourceView(m_colorBuffer.resource.Get(), &srvDesc, srvHandle);

    return S_OK;
}

HRESULT PreviewUI::CreateDepthBuffer(uint32_t width, uint32_t height)
{
    const auto& dev = m_dx12->GetDevice().Get();

    D3D12_HEAP_PROPERTIES heapProp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);

    D3D12_RESOURCE_DESC resDesc = CD3DX12_RESOURCE_DESC::Tex2D(
        DXGI_FORMAT_D32_FLOAT,
        width,
        height,
        1, 1, 1, 0,
        D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL);

    D3D12_CLEAR_VALUE cv = CD3DX12_CLEAR_VALUE(DXGI_FORMAT_D32_FLOAT, 1.0f, 0);

    HRESULT hr = dev->CreateCommittedResource(
        &heapProp,
        D3D12_HEAP_FLAG_NONE,
        &resDesc,
        D3D12_RESOURCE_STATE_DEPTH_WRITE,
        &cv,
        IID_PPV_ARGS(&m_depthBuffer.resource));

    if (FAILED(hr))
    {
        Debugger::Log("[Error] Failed to create Depth Buffer resource for Preview.\n");
        return hr;
    }

    m_depthBuffer.dsvHandle = m_gpuResMgr->AllocateDescriptor(nullptr, nullptr, HeapType::Dsv);

    D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
    dsvDesc.Format = DXGI_FORMAT_D32_FLOAT;
    dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
    dsvDesc.Flags = D3D12_DSV_FLAG_NONE;

    dev->CreateDepthStencilView(m_depthBuffer.resource.Get(), &dsvDesc, m_depthBuffer.dsvHandle);

    return S_OK;
}

void PreviewUI::ResizeBuffers(uint32_t width, uint32_t height)
{
    m_dx12->FlushCommandQueue();

    m_bufferWidth = width;
    m_bufferHeight = height;

    m_colorBuffer.resource.Reset();
    m_depthBuffer.resource.Reset();

    CreateColorBuffer(m_bufferWidth, m_bufferHeight);
    CreateDepthBuffer(m_bufferWidth, m_bufferHeight);
}