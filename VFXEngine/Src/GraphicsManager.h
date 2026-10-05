#pragma once
#include "ConstantBuffer.h"

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
			DirectX::XMFLOAT4X4 viewProj; // 64バイト (16バイト x 4)
			float globalDeltaTime;          // 4バイト
			float padding[3];
		}param{};
		struct DescIndices {
			float dummy[4];
		}descIndices{};
	}m_globalCBDesc{};

	ConstantBuffer<GlobalCBDesc> m_globalCB;

	HRESULT CreateGlobalCB(GpuResourceManager& gpuResMgr);

public:
	VFXManager* GetVFXMgr() { return m_vfxMgr.get(); }
	EditorGridPass* GetEditorGridPass() { return m_editorGridPass.get(); }

	struct InitDesc {
		Window& window;
		Dx12Wrapper& dx12;
		GpuResourceManager& gpuResMgr;
		TextureManager& texMgr;
	};
	void Init(const InitDesc& desc);
	
	void Execute();

	void ShutDown();
};