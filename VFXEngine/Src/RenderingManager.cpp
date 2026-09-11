#include "pch.h"
#include "RenderingManager.h"
#include "GpuResourceManager.h"

void RenderingManager::DrawImGui(ID3D12GraphicsCommandList* cmdList)
{
	// IMGui描画
	ImGui::Render();
	ID3D12DescriptorHeap* heaps[] = { m_gpuResMgr->GetDescHeap(HeapType::ImGui).heap.Get() };
	cmdList->SetDescriptorHeaps(1, heaps);
	ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), cmdList);
}