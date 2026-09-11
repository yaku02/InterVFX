#pragma once
#include "PipelineStructs.h"

class PSOBuilder {
public:
	static PipelineObject CreateComputePSO(ID3D12Device* dev, const ComputePSODesc& psoDesc);
	static PipelineObject CreateGraphicsPSO(ID3D12Device* dev, const GraphicsPSODesc& desc);
};