#pragma once

struct BackBuffer {
	const static int Count = 2;
	Microsoft::WRL::ComPtr<ID3D12Resource> resource = nullptr;
	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle{};
};