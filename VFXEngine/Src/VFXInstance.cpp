#include "pch.h"
#include "VFXInstance.h"
#include "VFXStructs.h"
#include "VFXAsset.h"
#include "GpuResourceManager.h"
#include "Dx12Wrapper.h"
#include "Debugger.h"

std::shared_ptr<VFXInstance> VFXInstance::Create(VFXAsset& asset, Dx12Wrapper& dx12, GpuResourceManager& gpuResMgr)
{
	auto inst = std::make_shared<VFXInstance>();
	if (FAILED(inst->CreateGpuResource(asset, dx12, gpuResMgr))) return nullptr;
	auto& baseInfo = inst->info.base;

	baseInfo.name = "VFX" + std::to_string(inst->info.Counter++);
	baseInfo.id = IDGenerator::Generate();
	baseInfo.type = InstType::VFX;

	inst->info.assetID = asset.info.base.id;
	inst->info.assetName = asset.info.base.name;

	inst->cpuParam.isActive = true;
	inst->cpuParam.world = Geometry::CreateWorldMatrix(inst->cpuParam.scale, inst->cpuParam.rotation, inst->cpuParam.position);
	inst->cpuParam.worldAABB = asset.cpuParam.localAABB.GetTransformed(inst->cpuParam.world);

	return inst;
}

HRESULT VFXInstance::CreateGpuResource(VFXAsset& asset, Dx12Wrapper& dx12, GpuResourceManager& gpuResMgr)
{
	const auto& dev = dx12.GetDevice().Get();
	const auto& cmdList = dx12.GetCmdList().Get();

	const auto numParticles = asset.gpuParam.numParticles;
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
			OutputDebugStringA("Creation particleBuffer failed\n");
			return result;
		}
		gpuResource.particleBuffer->SetName(L"particleBuffer");

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
			gpuResMgr.AllocateDescriptor(&gpuResource.particleSRVHandle, &gpuParam.indices.srvIndex);
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
			gpuResMgr.AllocateDescriptor(nullptr, &gpuParam.indices.uavIndex);

		dev->CreateUnorderedAccessView(
			gpuResource.particleBuffer.Get(), nullptr, &uavDesc, uavHandleCPU);
	}

	// 定数バッファ作成
	{
		uint32_t bufferSize = (sizeof(VFXInstance::GPUParam) + 0xff) & ~0xff;

		gpuResMgr.CreateConstantBuffer<VFXInstance::GPUParam>(
			gpuResource.constBuff,
			bufferSize,
			&gpuResource.constBuffMapData,
			L"VFX Inst ConstantBuffer"
		);

		gpuResource.cbvIndex = gpuResMgr.CreateCBV(gpuResource.constBuff, bufferSize);
	}
	return S_OK;
}