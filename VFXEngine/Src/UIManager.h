#pragma once

class Window;
class Dx12Wrapper;
class GpuResourceManager;
class BackBufferManager;
class SceneUI;

class UIManager {
private:
	Window* m_window = nullptr;
	Dx12Wrapper* m_dx12 = nullptr;
	GpuResourceManager* m_gpuResMgr = nullptr;
	BackBufferManager* m_backBufferMgr = nullptr;

	std::unique_ptr<SceneUI> m_sceneUI = nullptr;

	void ShowUIs();
public:
	UIManager();
	~UIManager();
	struct InitDesc {
		Window& window;
		Dx12Wrapper& dx12;
		GpuResourceManager& gpuResMgr;
		BackBufferManager& backBufferMgr;
	};
	void Init(InitDesc& initDesc);
	void ExecuteRendering();
	void ShutDown();
};