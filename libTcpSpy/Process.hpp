#ifndef PROCESS_HPP
#define PROCESS_HPP

#include <windef.h>
#include <Windows.h>
#include <psapi.h>

#include <memory>
#include <utility>
#include <string>

struct Process {
	using pointer = std::shared_ptr<Process>;

	Process() {}

	Process(DWORD pid)
		: m_pid(pid)
	{
	}

	Process(const Process& p) = delete;

	Process(Process&& p) noexcept
		: m_icon(p.m_icon), m_path(std::move(p.m_path)), m_name(std::move(p.m_name)), m_pid(p.m_pid)
	{
		p.m_icon = nullptr;
		p.m_pid = (DWORD)-1;
	}

	~Process() {
		if (m_icon) {
			DestroyIcon(m_icon);
			m_icon = nullptr;
		}
	}

	bool open() {
		HANDLE hProc = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, m_pid);

		if (hProc == NULL) {
			// failed to open process, could be: 1) not enough privileges, 2) opened process is very kernel-related and is not required for regular user to see
			HICON icon = LoadIcon(NULL, MAKEINTRESOURCE(IDI_APPLICATION));
			return false;
		}

		m_path = get_proc_path(hProc);

		m_name = extract_exe_name(m_path);

		m_icon = get_proc_icon(m_path);

		CloseHandle(hProc);

		return true;
	}

	DWORD m_pid{ (DWORD)-1 };
	HICON m_icon{ nullptr };
	std::wstring m_path{ L"Can not obtain ownership information" };
	std::wstring m_name{ L"Can not obtain ownership information" };

private:
	std::wstring get_proc_path(HANDLE hProc) {
		constexpr int proc_name_len = 512;

		WCHAR wProcName[proc_name_len];

		if (!GetModuleFileNameEx(hProc, NULL, wProcName, proc_name_len)) {
			return L"Can not obtain ownership information";
		}

		return std::wstring(wProcName);
	}

	HICON get_proc_icon(std::wstring filepath) {
		HICON icon = NULL;

		UINT res = ExtractIconEx(filepath.c_str(), 0, &icon, NULL, 1);

		if (res == UINT_MAX) {
			icon = LoadIcon(NULL, MAKEINTRESOURCE(IDI_APPLICATION));
			return icon;
		}

		return icon;
	}

	std::wstring extract_exe_name(const std::wstring& path) {
		LPCWSTR wProcPath = path.c_str();

		for (size_t i = path.size() - 1; i; i--) {
			if (wProcPath[i] == '\\') {
				return std::wstring(wProcPath + i + 1);
			}
		}

		return path;
	};
};

using ProcessPtr = Process::pointer;

#endif