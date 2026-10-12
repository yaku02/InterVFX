#include "pch.h"
#include "VFXInstance.h"
#include "VFXAsset.h"
#include "GpuResourceManager.h"
#include "Debugger.h"
#include "ConstantBuffer.h"

std::shared_ptr<VFXInstance> VFXInstance::Create(VFXAsset& asset, ID3D12Device* dev, ID3D12GraphicsCommandList* cmdList, GpuResourceManager& gpuResMgr)
{
	auto inst = std::make_shared<VFXInstance>();
	if (FAILED(inst->CreateGpuResource(asset, dev, cmdList, gpuResMgr))) return nullptr;
	auto& baseInfo = inst->info.base;

	baseInfo.name = "VFX" + std::to_string(VFXInstance::Info::Counter++);
	baseInfo.id = IDGenerator::Generate();
	baseInfo.type = InstType::VFX;

	inst->info.assetID = asset.info.base.id;
	inst->info.assetName = asset.info.base.name;

	inst->cpuParam.isActive = true;
	inst->cpuParam.world = Geometry::CreateWorldMatrix(inst->cpuParam.scale, inst->cpuParam.rotation, inst->cpuParam.position);
	inst->cpuParam.worldAABB = asset.cpuParam.localAABB.GetTransformed(inst->cpuParam.world);

	return inst;
}

HRESULT VFXInstance::CreateGpuResource(VFXAsset& asset, ID3D12Device* dev, ID3D12GraphicsCommandList* cmdList, GpuResourceManager& gpuResMgr)
{
	CreateParticleBuffer(asset, dev, cmdList, gpuResMgr);
	CreateDeadListBuffer(asset, dev, cmdList, gpuResMgr);
	CreateActiveListBuffer(asset, dev, cmdList, gpuResMgr);
	CreateIndirectArgsBuffer(asset, dev, cmdList, gpuResMgr);

	// 定数バッファ作成
	{
		gpuResource.cb = ConstantBuffer<CBDesc>::Create(gpuResMgr);
		if(gpuResource.cb.resource == nullptr || gpuResource.cb.mapData == nullptr)
		{
			Debugger::Log("Creation VFX Instance CB failed\n");
			return E_FAIL;
		}
	}
	return S_OK;
}

HRESULT VFXInstance::CreateParticleBuffer(VFXAsset& asset, ID3D12Device* dev, ID3D12GraphicsCommandList* cmdList, GpuResourceManager& gpuResMgr)
{
	const auto numParticles = asset.cbDesc.param.numParticles;

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

	for (uint32_t i = 0; i < numParticles; i++)
	{
		particles[i] = {};

		particles[i].age = asset.cbDesc.param.lifeTime + 1.0f;

		particles[i].seed =
			rand() * 747796405u + 2891336453u;
	}

	void* ptr = nullptr;
	gpuResource.uploadBuffer->Map(0, nullptr, &ptr);
	memcpy(ptr, particles.data(), sizeof(Particle) * numParticles);
	gpuResource.uploadBuffer->Unmap(0, nullptr);

	auto barrier1 = gpuResMgr.Transition(gpuResource.particleBuffer.Get(), ResState::Common, ResState::CopyDest);
	cmdList->ResourceBarrier(1, &barrier1);

	cmdList->CopyBufferRegion(gpuResource.particleBuffer.Get(), 0, gpuResource.uploadBuffer.Get(), 0, sizeof(Particle) * numParticles);

	// --- 修正箇所: CopyDest -> UnorderedAccess のバリアを発行 ---
	auto barrier2 = gpuResMgr.Transition(gpuResource.particleBuffer.Get(), ResState::CopyDest, ResState::UnorderedAccess);
	cmdList->ResourceBarrier(1, &barrier2);

	// SRV,UAVの作成
	cbDesc.descIndices.particleSRV = gpuResMgr.CreateSRV(gpuResource.particleBuffer, numParticles, sizeof(Particle));
	cbDesc.descIndices.particleUAV = gpuResMgr.CreateUAV(gpuResource.particleBuffer, numParticles, sizeof(Particle));

	return S_OK;
}

HRESULT VFXInstance::CreateDeadListBuffer(VFXAsset& asset, ID3D12Device* dev, ID3D12GraphicsCommandList* cmdList, GpuResourceManager& gpuResMgr)
{
	const auto numParticles = asset.cbDesc.param.numParticles;

	D3D12_HEAP_PROPERTIES heapProp =
		CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);

	// 要素数は純粋に numParticles 分だけでOK（カウンター領域は別にするため）
	uint32_t totalElements = numParticles;

	auto resDesc =
		CD3DX12_RESOURCE_DESC::Buffer(
			sizeof(uint32_t) * totalElements,
			D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS
		);

	auto result = dev->CreateCommittedResource(
		&heapProp,
		D3D12_HEAP_FLAG_NONE,
		&resDesc,
		D3D12_RESOURCE_STATE_COMMON,
		nullptr,
		IID_PPV_ARGS(&gpuResource.deadListBuffer));

	if (FAILED(result)) {
		OutputDebugStringA("Creation deadListBuffer failed\n");
		return result;
	}
	gpuResource.deadListBuffer->SetName(L"deadListBuffer");

	// カウンター用バッファ（4バイト）の作成
	auto counterResDesc = CD3DX12_RESOURCE_DESC::Buffer(
		sizeof(uint32_t),
		D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS
	);
	dev->CreateCommittedResource(
		&heapProp,
		D3D12_HEAP_FLAG_NONE,
		&counterResDesc,
		D3D12_RESOURCE_STATE_COMMON,
		nullptr,
		IID_PPV_ARGS(&gpuResource.deadCounterBuffer));
	gpuResource.deadCounterBuffer->SetName(L"deadListCounterBuffer");


	// --- 1. デッドリスト本体の初期化用配列（0, 1, 2, ..., numParticles-1） ---
	std::vector<uint32_t> initialDeadIndices(totalElements);
	for (uint32_t i = 0; i < numParticles; ++i) {
		initialDeadIndices[i] = i;
	}

	// --- 2. カウンター用の初期値（最初から全数が空き状態なので numParticles を入れる） ---
	uint32_t initialCounter = numParticles;


	// アップロード用バッファを使って GPU へ転送（本体用 ＋ カウンター用）
	heapProp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	auto uploadResDesc = CD3DX12_RESOURCE_DESC::Buffer(sizeof(uint32_t) * totalElements);

	dev->CreateCommittedResource(
		&heapProp,
		D3D12_HEAP_FLAG_NONE,
		&uploadResDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&gpuResource.deadUploadBuffer));

	void* ptr = nullptr; // ★ タイポを修正
	gpuResource.deadUploadBuffer->Map(0, nullptr, &ptr);
	memcpy(ptr, initialDeadIndices.data(), sizeof(uint32_t) * totalElements);
	gpuResource.deadUploadBuffer->Unmap(0, nullptr);

	// カウンター用のアップロードバッファ
	auto counterUploadResDesc = CD3DX12_RESOURCE_DESC::Buffer(sizeof(uint32_t));
	dev->CreateCommittedResource(
		&heapProp,
		D3D12_HEAP_FLAG_NONE,
		&counterUploadResDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&gpuResource.counterUploadBuffer));

	void* counterPtr = nullptr; // ★ タイポを修正
	gpuResource.counterUploadBuffer->Map(0, nullptr, &counterPtr);
	memcpy(counterPtr, &initialCounter, sizeof(uint32_t));
	gpuResource.counterUploadBuffer->Unmap(0, nullptr);


	// --- リソースバリア（COMMON -> COPY_DEST） ---
	CD3DX12_RESOURCE_BARRIER barriers[] = {
		gpuResMgr.Transition(gpuResource.deadListBuffer.Get(), ResState::Common, ResState::CopyDest),
		gpuResMgr.Transition(gpuResource.deadCounterBuffer.Get(), ResState::Common, ResState::CopyDest)
	};
	cmdList->ResourceBarrier(2, barriers);

	// コピー実行
	cmdList->CopyBufferRegion(gpuResource.deadListBuffer.Get(), 0, gpuResource.deadUploadBuffer.Get(), 0, sizeof(uint32_t) * totalElements);
	cmdList->CopyBufferRegion(gpuResource.deadCounterBuffer.Get(), 0, gpuResource.counterUploadBuffer.Get(), 0, sizeof(uint32_t));

	// --- リソースバリア（COPY_DEST -> UNORDERED_ACCESS） ---
	CD3DX12_RESOURCE_BARRIER barriersEnd[] = {
		gpuResMgr.Transition(gpuResource.deadListBuffer.Get(), ResState::CopyDest, ResState::UnorderedAccess),
		gpuResMgr.Transition(gpuResource.deadCounterBuffer.Get(), ResState::CopyDest, ResState::UnorderedAccess)
	};
	cmdList->ResourceBarrier(2, barriersEnd);


	// --- SRV, UAV の作成 ---
	cbDesc.descIndices.deadListSRV = gpuResMgr.CreateSRV(gpuResource.deadListBuffer, totalElements, sizeof(uint32_t));

	cbDesc.descIndices.deadListUAV = gpuResMgr.CreateAppendUAV(
		gpuResource.deadListBuffer,
		gpuResource.deadCounterBuffer,
		totalElements,
		sizeof(uint32_t)
	);

	return S_OK;
}

HRESULT VFXInstance::CreateActiveListBuffer(VFXAsset& asset, ID3D12Device* dev, ID3D12GraphicsCommandList* cmdList, GpuResourceManager& gpuResMgr)
{
	const auto numParticles = asset.cbDesc.param.numParticles;

	D3D12_HEAP_PROPERTIES heapProp =
		CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);

	uint32_t totalElements = numParticles;

	// 1. ActiveList 用バッファ（生存しているパーティクルのインデックスを格納）
	auto activeResDesc = CD3DX12_RESOURCE_DESC::Buffer(
		sizeof(uint32_t) * totalElements,
		D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS
	);

	dev->CreateCommittedResource(
		&heapProp,
		D3D12_HEAP_FLAG_NONE,
		&activeResDesc,
		D3D12_RESOURCE_STATE_COMMON, // COMMON で作成
		nullptr,
		IID_PPV_ARGS(&gpuResource.activeListBuffer));
	gpuResource.activeListBuffer->SetName(L"activeListBuffer");

	// 2. カウンター用バッファ（最初は生存数 0 なので 0 で初期化する）
	auto activeCounterResDesc = CD3DX12_RESOURCE_DESC::Buffer(
		sizeof(uint32_t),
		D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS
	);

	dev->CreateCommittedResource(
		&heapProp,
		D3D12_HEAP_FLAG_NONE,
		&activeCounterResDesc,
		D3D12_RESOURCE_STATE_COMMON, // COMMON で作成
		nullptr,
		IID_PPV_ARGS(&gpuResource.activeCounterBuffer));
	gpuResource.activeCounterBuffer->SetName(L"activeCounterBuffer");

	// --- カウンター初期化用（0 を書き込む） ---
	uint32_t initialCounter = 0;
	heapProp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	auto uploadResDesc = CD3DX12_RESOURCE_DESC::Buffer(sizeof(uint32_t));

	dev->CreateCommittedResource(
		&heapProp,
		D3D12_HEAP_FLAG_NONE,
		&uploadResDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&gpuResource.activeCountUploadBuffer));

	void* ptr = nullptr; // ★ タイポを修正
	gpuResource.activeCountUploadBuffer->Map(0, nullptr, &ptr);
	memcpy(ptr, &initialCounter, sizeof(uint32_t));
	gpuResource.activeCountUploadBuffer->Unmap(0, nullptr);

	// --- リソースバリア（COMMON -> COPY_DEST） ---
	CD3DX12_RESOURCE_BARRIER barriers[] = {
		gpuResMgr.Transition(gpuResource.activeListBuffer.Get(), ResState::Common, ResState::CopyDest),
		gpuResMgr.Transition(gpuResource.activeCounterBuffer.Get(), ResState::Common, ResState::CopyDest)
	};
	cmdList->ResourceBarrier(2, barriers);

	// カウンターの初期値（0）を GPU 側へコピー
	cmdList->CopyBufferRegion(gpuResource.activeCounterBuffer.Get(), 0, gpuResource.activeCountUploadBuffer.Get(), 0, sizeof(uint32_t));

	// --- リソースバリア（COPY_DEST -> UNORDERED_ACCESS） ---
	CD3DX12_RESOURCE_BARRIER barriersEnd[] = {
		gpuResMgr.Transition(gpuResource.activeListBuffer.Get(), ResState::CopyDest, ResState::UnorderedAccess),
		gpuResMgr.Transition(gpuResource.activeCounterBuffer.Get(), ResState::CopyDest, ResState::UnorderedAccess)
	};
	cmdList->ResourceBarrier(2, barriersEnd);

	// --- SRV, UAV の作成 ---
	cbDesc.descIndices.activeListUAV = gpuResMgr.CreateAppendUAV(
		gpuResource.activeListBuffer,
		gpuResource.activeCounterBuffer,
		totalElements,
		sizeof(uint32_t)
	);
	cbDesc.descIndices.activeListSRV = gpuResMgr.CreateSRV(
		gpuResource.activeListBuffer,
		totalElements,
		sizeof(uint32_t)
	);
	cbDesc.descIndices.activeCounterUAV = gpuResMgr.CreateUAV(
		gpuResource.activeCounterBuffer,
		1,
		sizeof(uint32_t)
	);

	return S_OK;
}

HRESULT VFXInstance::CreateIndirectArgsBuffer(VFXAsset& asset, ID3D12Device* dev, ID3D12GraphicsCommandList* cmdList, GpuResourceManager& gpuResMgr)
{
	struct DrawArguments {
		uint32_t vertexCountPerInstance; // 4 (四角形なら4)
		uint32_t instanceCount;          // ここをGPU側（CopyArgsCS）でアクティブ数に書き換える
		uint32_t startVertexLocation;    // 0
		uint32_t startInstanceLocation;  // 0
	};

	D3D12_HEAP_PROPERTIES heapProp =
		CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);

	auto argsResDesc = CD3DX12_RESOURCE_DESC::Buffer(
		sizeof(DrawArguments),
		D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS // GPUから書き換えるためUAVフラグが必要
	);

	// 1. COMMON で作成
	dev->CreateCommittedResource(
		&heapProp,
		D3D12_HEAP_FLAG_NONE,
		&argsResDesc,
		D3D12_RESOURCE_STATE_COMMON,
		nullptr,
		IID_PPV_ARGS(&gpuResource.indirectArgsBuffer));
	gpuResource.indirectArgsBuffer->SetName(L"indirectArgsBuffer");

	// --- 初期値（頂点数4、インスタンス数0）の準備 ---
	DrawArguments initialArgs = {};
	initialArgs.vertexCountPerInstance = 4;
	initialArgs.instanceCount = 0;
	initialArgs.startVertexLocation = 0;
	initialArgs.startInstanceLocation = 0;

	// アップロード用バッファを作成して初期値を転送
	heapProp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	auto uploadResDesc = CD3DX12_RESOURCE_DESC::Buffer(sizeof(DrawArguments));

	dev->CreateCommittedResource(
		&heapProp,
		D3D12_HEAP_FLAG_NONE,
		&uploadResDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&gpuResource.argsUploadBuffer));

	void* ptr = nullptr;
	gpuResource.argsUploadBuffer->Map(0, nullptr, &ptr);
	memcpy(ptr, &initialArgs, sizeof(DrawArguments));
	gpuResource.argsUploadBuffer->Unmap(0, nullptr);

	// --- ① 描画準備：COMMON -> COPY_DEST へ遷移 ---
	gpuResMgr.Transition(cmdList, gpuResource.indirectArgsBuffer.Get(), ResState::Common, ResState::CopyDest);

	// 初期値を GPU バッファへコピー
	cmdList->CopyBufferRegion(gpuResource.indirectArgsBuffer.Get(), 0, gpuResource.argsUploadBuffer.Get(), 0, sizeof(DrawArguments));

	// --- ② 描画準備：COPY_DEST -> UNORDERED_ACCESS へ遷移（次フレームの CopyArgsCS に備える） ---
	gpuResMgr.Transition(cmdList, gpuResource.indirectArgsBuffer.Get(), ResState::CopyDest, ResState::UnorderedAccess);

	// UAV作成
	uint32_t stride = sizeof(uint32_t) * 4;
	uint32_t numElements = 1;

	cbDesc.descIndices.indirectArgsUAV = gpuResMgr.CreateUAV(
		gpuResource.indirectArgsBuffer,
		numElements,
		stride
	);

	return S_OK;
}