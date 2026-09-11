#include "pch.h"
#include "Debugger.h"
#include "LogStream.h"

void Debugger::Log(const char* format, ...)
{
#ifdef _DEBUG
    va_list valist;
    va_start(valist, format);

    // 1. 必要サイズの計算
    int size = vsnprintf(nullptr, 0, format, valist);
    if (size > 0)
    {
        std::string buffer(size + 1, '\0');
        vsnprintf(&buffer[0], buffer.size(), format, valist);

        // 2. Visual Studio の出力ウィンドウへ送信
        OutputDebugStringA(buffer.c_str());

        // 3. ImGui のログバッファへ送信 (★追加)
        LogStream::Get().AddLog(buffer);
    }

    va_end(valist);

#endif
}

void Debugger::EnableDebugLayer()
{
	ID3D12Debug* debugLayer = nullptr;
	auto result = D3D12GetDebugInterface(
	IID_PPV_ARGS(&debugLayer));
	debugLayer->EnableDebugLayer();
	debugLayer->Release();
}