#pragma once

class ShaderCompiler
{
	template <typename T>
	using ComPtr = Microsoft::WRL::ComPtr<T>;

public:
	ShaderCompiler();
	~ShaderCompiler();
	ComPtr<IDxcBlob> CompileShader(const std::string& filePath, const std::string& entryPoint, const std::string& target);
};