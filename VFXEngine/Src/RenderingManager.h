#pragma once
#include "Gbuffer.h"
#include "HdrBuffer.h"

enum class RtvType {
	Gbuffer,
	HdrBuffer,
};

class GpuResourceManager;
class RenderingManager {
	template <typename T>
	using ComPtr = Microsoft::WRL::ComPtr<T>;

private:
	ComPtr<ID3D12Device> m_dev = nullptr;
	GpuResourceManager* m_gpuResMgr = nullptr;
	Gbuffer m_gbuffer;
	HdrBuffer m_hdrBuffer;

public:
	void Init(ID3D12Device* dev) {
		m_dev = dev;
	}
	void DrawImGui(ID3D12GraphicsCommandList* cmdList);
	void SetRenderTargetWithDepth(ID3D12GraphicsCommandList* cmdList, RtvType type);

};