#pragma once
#include "TextureStructs.h"

class Dx12Wrapper;
class GpuResourceManager;

class TextureManager {
	template <typename T>
	using ComPtr = Microsoft::WRL::ComPtr<T>;

private:
	Dx12Wrapper* m_dx12 = nullptr;
	GpuResourceManager* m_gpuResMgr = nullptr;

	inline static Texture m_whiteTex;

	std::vector<ComPtr<ID3D12Resource>> m_tempUploadBuffers{};

public:
	~TextureManager() {
		m_whiteTex.buffer.Reset();
		m_tempUploadBuffers.clear();
	}

	struct InitDesc {
		Dx12Wrapper& dx12;
		GpuResourceManager& gpuResMgr;
	};

	void Init(const InitDesc& initDesc);

	HRESULT CreateWhiteTex();

	Texture LoadPNG(const std::string& filePath);

	static const Texture& GetWhiteTex() { return m_whiteTex; }
};