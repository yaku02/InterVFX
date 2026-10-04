#include "pch.h"
#include "VFXAsset.h"
#include "Dx12Wrapper.h"
#include "GpuResourceManager.h"
#include "IDGenerator.h"

std::shared_ptr<VFXAsset> VFXAsset::Create(ID3D12Device* dev, GpuResourceManager& gpuResMgr)
{
	auto asset = std::make_shared<VFXAsset>();
	auto& cpuParam = asset->cpuParam;

	// 生成した asset インスタンス経由で呼び出す
	if (FAILED(asset->CreateGpuResource(gpuResMgr)))
	{
		return nullptr; // リソース作成失敗時は nullptr を返す
	}
	asset->info.base.type = AssetType::VFX;
	asset->info.base.instType = InstType::VFX;
	asset->info.base.name = "VFXAsset" + std::to_string(VFXAsset::Info::Counter++);
	asset->info.base.id = IDGenerator::Generate();
	cpuParam.localAABB.min.x = cpuParam.minLocalAABB[0];
	cpuParam.localAABB.min.y = cpuParam.minLocalAABB[1];
	cpuParam.localAABB.min.z = cpuParam.minLocalAABB[2];

	cpuParam.localAABB.max.x = cpuParam.maxLocalAABB[0];
	cpuParam.localAABB.max.y = cpuParam.maxLocalAABB[1];
	cpuParam.localAABB.max.z = cpuParam.maxLocalAABB[2];

	return asset;
}

HRESULT VFXAsset::CreateGpuResource(GpuResourceManager& gpuResMgr)
{
	gpuResource.cb = ConstantBuffer<CBDesc>::Create(gpuResMgr);
	if (gpuResource.cb.resource == nullptr || gpuResource.cb.mapData == nullptr)
	{
		Debugger::Log("Creation VFX asset cb failed\n");
		return E_FAIL;
	}
}