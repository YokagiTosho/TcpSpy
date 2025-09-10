#ifndef CLIPBOARD_HPP
#define CLIPBOARD_HPP

#include "framework.h"

#include <cassert>

namespace Clipboard {
	inline bool CopyStrToClipboard(LPCWSTR buf) {
		int buf_real_bytes_len = (wcslen(buf) + 1) * 2; // multiply by two because of wchar_t, also add 1 for null terminating symbol

		HGLOBAL hCopyMem = GlobalAlloc(GMEM_MOVEABLE, buf_real_bytes_len);
		if (!hCopyMem) {
			return 0;
		}

		LPWSTR clipboard_buf = (LPWSTR)GlobalLock(hCopyMem);
		if (!clipboard_buf) {
			return 0;
		}

		memcpy(clipboard_buf, buf, buf_real_bytes_len);

		if (!GlobalUnlock(hCopyMem) && GetLastError() != NO_ERROR) {
			return 0;
		}

		if (!OpenClipboard(NULL)) {
			return 0;
		}

		if (!EmptyClipboard()) {
			return 0;
		}

		if (!SetClipboardData(CF_UNICODETEXT, hCopyMem)) {
			return 0;
		}

		if (!CloseClipboard()) {
			return 0;
		}

		return 1;
	}
}


#endif