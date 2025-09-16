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
			goto Error;
		}

		memcpy(clipboard_buf, buf, buf_real_bytes_len);

		if (!GlobalUnlock(hCopyMem) && GetLastError() != NO_ERROR) {
			goto Error;
		}

		if (!OpenClipboard(NULL)) {
			goto Error;
		}

		if (!EmptyClipboard()) {
			goto Error;
		}

		if (!SetClipboardData(CF_UNICODETEXT, hCopyMem)) {
			goto Error;
		}

		if (!CloseClipboard()) {
			return 0; // no need to free memory here, it has been moved to clipboard anyway
		}

		// hCopyMem is not freed, because its ownership gets moved into clipboard

		return 1;
	Error:
		GlobalFree(hCopyMem);
		return 0;
	}
}


#endif