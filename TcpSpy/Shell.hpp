#ifndef SHELL_HPP
#define SHELL_HPP

#include <commdlg.h>
#include <shlobj_core.h>

#include "framework.h"

namespace MShell {
	inline void Properties(HWND hWnd, LPCWSTR lpPath) {
		SHELLEXECUTEINFO sei{};
		sei.cbSize = { sizeof(SHELLEXECUTEINFO) };
		sei.fMask = SEE_MASK_INVOKEIDLIST;
		sei.hwnd = hWnd;
		sei.lpVerb = L"properties";
		sei.lpFile = lpPath;

		ShellExecuteEx(&sei);
	}

	inline bool GetSaveFilePath(HWND hWnd, LPWSTR _Inout_ lpFileBuf, int fileBufLen) {
		OPENFILENAME ofn{};
		bool ret = true;

		ofn.lStructSize = sizeof(OPENFILENAME);
		ofn.hwndOwner = hWnd;
		ofn.lpstrFilter = L".csv";
		ofn.lpstrCustomFilter = NULL;

		ofn.lpstrFile = lpFileBuf;
		ofn.nMaxFile = fileBufLen;
		PWSTR dirPathBuf;
		if (SHGetKnownFolderPath(FOLDERID_Desktop, 0, NULL, &dirPathBuf) != S_OK) {
			ret = false;
			goto exit;
		}

		ofn.lpstrInitialDir = dirPathBuf;
		ofn.lpstrTitle = NULL;

		ofn.Flags = OFN_OVERWRITEPROMPT;

		if (!GetSaveFileName(&ofn)) {
			ret = false;
			goto exit;
		}

	exit:
		CoTaskMemFree(dirPathBuf);
		return ret;
	}
}


#endif