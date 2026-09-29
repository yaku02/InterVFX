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
	if (FAILED(asset->CreateGpuResource(dev, gpuResMgr)))
	{
		return nullptr; // リソース作成失敗時は nullptr を返す
	}
	asset->info.base.type = AssetType::VFX;
	asset->info.base.instType = InstType::VFX;
	asset->info.base.name = "VFXAsset" + std::to_string(asset->info.Counter++);
	asset->info.base.id = IDGenerator::Generate();
	cpuParam.localAABB.min.x = cpuParam.minLocalAABB[0];
	cpuParam.localAABB.min.y = cpuParam.minLocalAABB[1];
	cpuParam.localAABB.min.z = cpuParam.minLocalAABB[2];

	cpuParam.localAABB.max.x = cpuParam.maxLocalAABB[0];
	cpuParam.localAABB.max.y = cpuParam.maxLocalAABB[1];
	cpuParam.localAABB.max.z = cpuParam.maxLocalAABB[2];

	return asset;
}

HRESULT VFXAsset::CreateGpuResource(ID3D12Device* dev, GpuResourceManager& gpuResMgr)
{
	// ベースアセットパラメータ定数バッファ作成
	{
		uint32_t bufferSize = (sizeof(VFXAsset::GPUParam) + 0xff) & ~0xff;

		gpuResMgr.CreateConstantBuffer<VFXAsset::GPUParam>(
			gpuResource.constBuff,
			bufferSize,
			&gpuResource.mapData,
			L"VFX Asset ConstantBuffer"
		);

		gpuResource.cbvIndex = gpuResMgr.CreateCBV(gpuResource.constBuff, bufferSize);

	}
	return S_OK;
}