#include "pch.h"
#include "VFXPreview.h"
#include "VFXStructs.h"
#include "VFXAsset.h"
#include "Debugger.h"

HRESULT VFXPreview::CreateGpuResource(VFXAsset& asset, ID3D12Device* dev, ID3D12GraphicsCommandList* cmdList, GpuResourceManager& gpuResMgr)
{
	const auto numParticles = asset.cbDesc.param.numParticles;
	// パーティクルバッファ作成
	{
		D3D12_HEAP_PROPERTIES heapProp =
			CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);

		auto resDesc =
			CD3DX12_RESOURCE_DESC::Buffer(
				sizeof(Particle) * numParticles,
				D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS
			);

		auto result = dev->CreateCommittedResource(
			&heapProp,
			D3D12_HEAP_FLAG_NONE,
			&resDesc,
			D3D12_RESOURCE_STATE_COMMON,
			nullptr,
			IID_PPV_ARGS(&gpuResource.particleBuffer));

		if (FAILED(result)) {
			OutputDebugStringA("Creation Preview particleBuffer failed\n");
			return result;
		}
		gpuResource.particleBuffer->SetName(L"Preview particleBuffer");

		// UPLOADバッファの作成
		heapProp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
		resDesc = CD3DX12_RESOURCE_DESC::Buffer(sizeof(Particle) * numParticles);

		dev->CreateCommittedResource(
			&heapProp,
			D3D12_HEAP_FLAG_NONE,
			&resDesc,
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&gpuResource.uploadBuffer));

		std::vector<Particle> particles;

		particles.resize(numParticles);
		for (uint32_t i = 0; i < numParticles; ++i)
		{
			particles[i].seed =
				rand() * 747796405u + 2891336453u;
		}

		void* ptr = nullptr;
		gpuResource.uploadBuffer->Map(0, nullptr, &ptr);
		memcpy(ptr, particles.data(), sizeof(Particle) * numParticles);
		gpuResource.uploadBuffer->Unmap(0, nullptr);

		auto barrierDesc = CD3DX12_RESOURCE_BARRIER::Transition(
			gpuResource.particleBuffer.Get(),
			D3D12_RESOURCE_STATE_COMMON,
			D3D12_RESOURCE_STATE_COPY_DEST);
		cmdList->ResourceBarrier(1, &barrierDesc);

		cmdList->CopyBufferRegion(gpuResource.particleBuffer.Get(), 0, gpuResource.uploadBuffer.Get(), 0, sizeof(Particle) * numParticles);

		auto barrierEnd = CD3DX12_RESOURCE_BARRIER::Transition(
			gpuResource.particleBuffer.Get(),
			D3D12_RESOURCE_STATE_COPY_DEST,
			D3D12_RESOURCE_STATE_UNORDERED_ACCESS); // CSで更新するためにUAVとして
		cmdList->ResourceBarrier(1, &barrierEnd);

		// SRVの作成
		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
		srvDesc.Format = DXGI_FORMAT_UNKNOWN;
		srvDesc.Buffer.FirstElement = 0;
		srvDesc.Buffer.NumElements = numParticles;
		srvDesc.Buffer.StructureByteStride = sizeof(Particle);
		srvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;

		D3D12_CPU_DESCRIPTOR_HANDLE srvHandleCPU =
			gpuResMgr.AllocateDescriptor(nullptr, &cbDesc.descIndices.particleSRV);
		dev->CreateShaderResourceView(gpuResource.particleBuffer.Get(), &srvDesc, srvHandleCPU);

		// UAVの作成
		D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
		uavDesc.Format = DXGI_FORMAT_UNKNOWN;
		uavDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
		uavDesc.Buffer.FirstElement = 0;
		uavDesc.Buffer.NumElements = numParticles;
		uavDesc.Buffer.StructureByteStride = sizeof(Particle);
		uavDesc.Buffer.CounterOffsetInBytes = 0;
		uavDesc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_NONE;

		D3D12_CPU_DESCRIPTOR_HANDLE uavHandleCPU =
			gpuResMgr.AllocateDescriptor(nullptr, &cbDesc.descIndices.particleUAV);

		dev->CreateUnorderedAccessView(
			gpuResource.particleBuffer.Get(), nullptr, &uavDesc, uavHandleCPU);
	}

	// 定数バッファ作成
	{
		gpuResource.cb = ConstantBuffer<CBDesc>::Create(gpuResMgr);
		if (gpuResource.cb.resource == nullptr || gpuResource.cb.mapData == nullptr)
		{
			Debugger::Log("Creation VFX Preview CB failed\n");
			return E_FAIL;
		}
	}
	return S_OK;
}