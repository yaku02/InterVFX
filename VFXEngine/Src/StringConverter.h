#pragma once

class StringConverter {
public:
	static std::wstring ToWString(const std::string& str) {
		if (str.empty()) {
			return L"";
		}

		// 必要なバッファサイズを取得
		int sizeNeeded = MultiByteToWideChar(
			CP_UTF8,          // UTF-8からの変換（Shift_JISなどの場合は CP_ACP に変更）
			0,
			str.c_str(),
			static_cast<int>(str.size()),
			nullptr,
			0
		);

		if (sizeNeeded <= 0) {
			return L"";
		}

		std::wstring result(sizeNeeded, 0);
		MultiByteToWideChar(
			CP_UTF8,
			0,
			str.c_str(),
			static_cast<int>(str.size()),
			&result[0],
			sizeNeeded
		);

		return result;
	}

	// LPCWSTR を取得する利便用ヘルパー（内部で ToWString を呼ぶ）
	// 注意: 戻り値の LPCWSTR は、呼び出し元のスコープで使用してください
	static std::wstring ToLPCWSTR(const std::string& str) {
		return ToWString(str);
	}
};