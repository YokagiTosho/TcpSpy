#ifndef STATUS_BAR_HPP
#define STATUS_BAR_HPP

#include "framework.h"

#include "libTcpSpy/ConnectionsTableManager.hpp"

#include <memory>

class StatusBar {
public:
	using pointer = std::unique_ptr<StatusBar>;

	StatusBar(HWND parent) 
		: m_parent(parent)
	{
		m_sb = CreateWindow(
			STATUSCLASSNAME,
			L"",
			m_styles,
			0,
			0,
			0,
			0,
			parent,
			NULL,
			NULL,
			NULL
		);

		ShowWindow(m_sb, SW_SHOW);
		
		init();
	}

	void resize() {
		// status bar should automatically gets resized
		MoveWindow(m_sb,
			0,0,0,0,
			TRUE
		);
		SendMessage(m_sb, WM_SIZE, 0, 0);
	}

	void set_items(const std::vector<std::wstring> &items) {
		if (items.size() < m_parts) {
			return;
		}

		for (int i = 0; i < m_parts; i++) {
			set_text(i, items[i].c_str());
		}

		resize();
	}

	void update(ConnectionsTableManager &ctm) {
		int counter[5] = { 0,0,0,0,0 };

		counter[0] = ctm.size();
		for (auto& row : ctm) {
			switch (row->address_family()) {
			case ProtocolFamily::INET:
				counter[1]++;
				break;
			case ProtocolFamily::INET6:
				counter[2]++;
				break;
			}

			switch (row->protocol()) {
			case ConnectionProtocol::PROTO_TCP:
				counter[3]++;
				break;
			case ConnectionProtocol::PROTO_UDP:
				counter[4]++;
				break;
			}
		}

		set_items({
			std::format(L"All: {}", counter[0]),
			std::format(L"TCP: {}", counter[3]),
			std::format(L"UDP: {}", counter[4]),
			std::format(L"IPv4: {}", counter[1]),
			std::format(L"IPv6: {}", counter[2]),
			});
	}

	~StatusBar() {
		if (m_sb) {
			DestroyWindow(m_sb);
			m_sb = NULL;
		}
	}
private:
	int set_text(char index, LPCWSTR text) {
		WPARAM wParam = MAKEWPARAM(MAKEWORD(index, SBT_NOBORDERS), 0);
		return SendMessage(m_sb, SB_SETTEXT, wParam, (LPARAM)text);
	}

	void init() {
		RECT rcClient;
		HLOCAL hloc;
		PINT paParts;
		int i, nWidth;

		GetClientRect(m_parent, &rcClient);

		hloc = LocalAlloc(LHND, sizeof(int) * m_parts);
		paParts = (PINT)LocalLock(hloc);

		nWidth = rcClient.right / m_parts;
		int rightEdge = nWidth;
		for (i = 0; i < m_parts; i++) {
			paParts[i] = rightEdge;
			rightEdge += nWidth;
		}

		SendMessage(m_sb, SB_SETPARTS, (WPARAM)m_parts, (LPARAM)
			paParts);

		LocalUnlock(hloc);
		LocalFree(hloc);
	}
	HWND m_sb{ NULL };
	HWND m_parent{ NULL };

	DWORD m_styles{ WS_CHILD };

	int m_parts{ 5 };
};

#endif