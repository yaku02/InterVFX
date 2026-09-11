#pragma once

struct Texture {
	Microsoft::WRL::ComPtr<ID3D12Resource> buffer = nullptr;
	D3D12_GPU_DESCRIPTOR_HANDLE engineSrvHandle{};
	D3D12_GPU_DESCRIPTOR_HANDLE imguiSrvHandle{};
};