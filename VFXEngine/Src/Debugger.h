#pragma once

class Debugger
{
public:
	static void Log(const char* format, ...);
	void EnableDebugLayer();
};

