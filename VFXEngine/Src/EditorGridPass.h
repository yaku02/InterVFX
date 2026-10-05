#pragma once
#include "EditorGridRenderer.h"

class Dx12Wrapper;
class GpuResourceManager;
class EditorGridPass {
private:
	std::unique_ptr<EditorGridRenderer> m_renderer = nullptr;

public:
	struct InitDesc {
		ID3D12Device* dev;
		GpuResourceManager& gpuResMgr;
	};
	void Init(const InitDesc& desc);

	struct PassExecuteDesc {
		EditorGridRenderer::RenderDesc renderDesc;
	};

	void Execute(const PassExecuteDesc& desc);
};