#pragma once

class Window;
class Dx12Wrapper;
class GpuResourceManager;
class BackBufferManager;
class VFXAssetEditor;
class VFXInstEditor;

class SceneUI;
class AssetUI;
class InstUI;
class PreviewUI;
class UIManager {
private:
	Window* m_window = nullptr;
	Dx12Wrapper* m_dx12 = nullptr;
	GpuResourceManager* m_gpuResMgr = nullptr;
	BackBufferManager* m_backBufferMgr = nullptr;
	VFXAssetEditor* m_vfxAssetEditor = nullptr;
	VFXInstEditor* m_vfxInstEditor = nullptr;

	std::unique_ptr<SceneUI> m_sceneUI = nullptr;
	std::unique_ptr<AssetUI> m_assetUI = nullptr;
	std::unique_ptr<InstUI> m_instUI = nullptr;
	std::unique_ptr<PreviewUI> m_previewUI = nullptr;

	void ShowUIs();
public:
	UIManager();
	~UIManager();
	struct InitDesc {
		Window& window;
		Dx12Wrapper& dx12;
		GpuResourceManager& gpuResMgr;
		BackBufferManager& backBufferMgr;
		VFXAssetEditor* vfxAssetEditor;
		VFXInstEditor* vfxInstEditor;
	};
	void Init(const InitDesc& initDesc);
	void ExecuteRendering();
	void ShutDown();
};