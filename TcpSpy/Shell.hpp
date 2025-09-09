#ifndef SHELL_HPP
#define SHELL_HPP

#include "framework.h"

namespace Shell {
	inline void Properties(HWND hWnd, LPCWSTR lpPath) {
		SHELLEXECUTEINFO sei{};
		sei.cbSize = { sizeof(SHELLEXECUTEINFO) };
		sei.fMask = SEE_MASK_INVOKEIDLIST;
		sei.hwnd = hWnd;
		sei.lpVerb = L"properties";
		sei.lpFile = lpPath;

		ShellExecuteEx(&sei);
	}
}


#endif