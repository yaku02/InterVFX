#include "pch.h"

#include "ShaderCompiler.h"
#include "Debugger.h"

template <typename T>
using ComPtr = Microsoft::WRL::ComPtr<T>;

ShaderCompiler::ShaderCompiler()
{
}

ShaderCompiler::~ShaderCompiler()
{
}

ComPtr<IDxcBlob> ShaderCompiler::CompileShader(const std::string& filePath, const std::string& entryPoint, const std::string& target)
{
	namespace fs = std::filesystem;

	std::wstring wFilename = fs::path(filePath).wstring();
	std::wstring wEntryPoint = fs::path(entryPoint).wstring();
	std::wstring wTarget = fs::path(target).wstring();

	ComPtr<IDxcUtils> utils;
	ComPtr<IDxcCompiler3> compiler;

	DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&utils));
	DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&compiler));

	// デフォルトのインクルードハンドラを作成
	ComPtr<IDxcIncludeHandler> includeHandler;
	utils->CreateDefaultIncludeHandler(&includeHandler);

	if (!fs::exists(filePath)) {
		std::string errorMsg = "[Shader Error] File NOT found: " + filePath + "\n";

		// 絶対パスを表示して、プログラムが「どこを探しに行って失敗したか」を暴く
		errorMsg += "Looked at: " + fs::absolute(filePath).string() + "\n";

		OutputDebugStringA(errorMsg.c_str());
		return nullptr; // 存在しないならこれ以上進まない
	}

	ComPtr<IDxcBlobEncoding> source;
	// LoadFileの戻り値（HRESULT）も受け取ってチェックできるようにする
	HRESULT hr = utils->LoadFile(wFilename.c_str(), nullptr, &source);

	if (FAILED(hr) || !source) {
		std::string errorMsg = "[Shader Error] Failed to LoadFile: " + filePath + "\n";
		OutputDebugStringA(errorMsg.c_str());
		return nullptr;
	}

	DxcBuffer buffer = {};
	buffer.Ptr = source->GetBufferPointer();
	buffer.Size = source->GetBufferSize();
	
	fs::path absoluteShaderPath = fs::absolute("Engine/Shader");
	std::wstring commonShaderPath = absoluteShaderPath.wstring();

	std::filesystem::path parentPath = fs::path(wFilename).parent_path();
	std::wstring wIncludePath = parentPath.wstring();

	std::vector<LPCWSTR> args = {
		L"-E", wEntryPoint.c_str(),
		L"-T", wTarget.c_str(),
		L"-Zi",
		L"-Qembed_debug",
		L"-I", commonShaderPath.c_str(),
		L"-I", wIncludePath.c_str()
	};
	
	ComPtr<IDxcResult> loadResult;
	compiler->Compile(
		&buffer, 
		args.data(),
		static_cast<UINT32>(args.size()), // 引数の個数
		includeHandler.Get(),
		IID_PPV_ARGS(&loadResult));

	// 1. コンパイル結果のステータス（成功したかどうか）を厳密にチェックする
	HRESULT compileStatus = S_OK;
	loadResult->GetStatus(&compileStatus);

	// 2. エラーログの抽出
	ComPtr<IDxcBlobUtf8> errorblob;
	loadResult->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(&errorblob), nullptr);

	if (errorblob && errorblob->GetStringLength() > 0) {
		// 出力ウィンドウにエラーの詳細（何行目がエラーかなど）を流す
		Debugger::Log("\n--- [HLSL Compile Error Log] ---\n");
		Debugger::Log(errorblob->GetStringPointer());
	}

	// 3. コンパイル自体が失敗（StatusがFAILED）なら、OBJECTを取りに行かずに即リターン
	if (FAILED(compileStatus)) {
		std::string msg = "[Shader Error] Compile failed for: " + filePath + "\n";
		Debugger::Log(msg.c_str());
		return nullptr;
	}

	ComPtr<IDxcBlob> blob;
	HRESULT result = loadResult->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&blob), nullptr);

	if (FAILED(result))
	{
		std::string msg = filePath + " failed to get DXC_OUT_OBJECT\n";
		Debugger::Log(msg.c_str());
		return nullptr;
	}

	return blob;
}