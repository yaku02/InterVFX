#pragma once

// Standard Library
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <unordered_map>
#include <map>
#include <queue>
#include <functional>
#include <memory>
#include <atomic>
#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <cstdarg>
#include <cmath>
#include <cfloat>

// Windows
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <Windows.h>
#include <windowsx.h>
#include <tchar.h>
#include <wrl/client.h>
#include <dwmapi.h>

// DirectX 12
#include <d3d12.h>
#include <dxgi1_6.h>
#include <dxgidebug.h>
#include <DirectXMath.h>
#include <d3dx12.h>
#include <dxcapi.h>

// Libraries
#include <json.hpp>

#include <DDSTextureLoader.h>
#include <ResourceUploadBatch.h>
#include <WICTextureLoader.h>

#include <happly.h>

#include <imgui.h>
#include <imgui_impl_win32.h>
#include <imgui_impl_dx12.h>