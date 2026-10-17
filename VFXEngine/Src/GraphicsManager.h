#pragma once
#include "ConstantBuffer.h"
#include "RenderStructs.h"
#include "CameraStructs.h"

class Window;
class Dx12Wrapper;
class GpuResourceManager;
class TextureManager;

class VFXManager;
class EditorGridPass;
class GraphicsManager {
private:
	Window* m_window = nullptr;
	Dx12Wrapper* m_dx12 = nullptr;
	GpuResourceManager* m_gpuResMgr = nullptr;
	TextureManager* m_texMgr = nullptr;
	std::unique_ptr<VFXManager> m_vfxMgr = nullptr;
	std::unique_ptr<EditorGridPass> m_editorGridPass = nullptr;

	struct GlobalCBDesc {
		struct Param{
			CameraParam camera{};
			float               globalDeltaTime; // 4バイト (計16バイトのブロックに収まる)
			float padding[3];
		}param{};
		struct DescIndices {
			float dummy[4];
		}descIndices{};
	}m_globalCBDesc{};

	ConstantBuffer<GlobalCBDesc> m_globalCB;

	HRESULT CreateGlobalCB(GpuResourceManager& gpuResMgr);

public:
	GraphicsManager();             
	~GraphicsManager();             

	VFXManager* GetVFXMgr();
	EditorGridPass* GetEditorGridPass();

	struct InitDesc {
		Window& window;
		Dx12Wrapper& dx12;
		GpuResourceManager& gpuResMgr;
		TextureManager& texMgr;
	};
	void Init(const InitDesc& desc);
	
	void Execute(RenderMode mode, CameraParam& param);

	void ShutDown();
};