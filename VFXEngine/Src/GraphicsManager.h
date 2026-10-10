#pragma once
#include "ConstantBuffer.h"
#include "RenderStructs.h"

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
			DirectX::XMFLOAT4X4 viewProj;    // 64バイト (Matrix)
			DirectX::XMFLOAT4X4 invViewProj; // 64バイト (★追加: レイ復元用)
			DirectX::XMFLOAT3   cameraPos;   // 12バイト (★追加: グリッド視線計算用)
			float               globalDeltaTime; // 4バイト (計16バイトのブロックに収まる)
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
	
	void Execute(RenderMode mode);

	void ShutDown();
};