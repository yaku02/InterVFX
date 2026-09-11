#pragma once
#include "TextureStructs.h"

class Dx12Wrapper;
class GpuResourceManager;

class TextureManager {
private:
	Dx12Wrapper* m_dx12 = nullptr;
	GpuResourceManager* m_gpuResMgr = nullptr;

	inline static Texture m_whiteTex;

public:
	struct InitDesc {
		Dx12Wrapper& dx12;
		GpuResourceManager& gpuResMgr;
	};

	void Init(const InitDesc& initDesc);

	HRESULT CreateWhiteTex();

	static const Texture& GetWhiteTex() { return m_whiteTex; }
};