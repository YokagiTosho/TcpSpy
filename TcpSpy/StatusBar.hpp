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
			0, 0, 0, 0,
			parent,
			NULL, NULL, NULL
		);

		init();
		get_sb_height();

		ShowWindow(m_sb, SW_SHOW);
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
		assert(items.size() >= m_parts);

		for (int i = 0; i < m_parts; i++) {
			set_text(i, items[i].c_str());
		}

		resize();
	}

	void update(ConnectionsTableManager &ctm) {
		enum {
			ALL_COUNT,
			INET_COUNT,
			INET6_COUNT,
			TCP_COUNT,
			UDP_COUNT,
			COUNT, 
		};
		
		int counter[COUNT]{};

		counter[ALL_COUNT] = ctm.size();

		for (auto& row : ctm) {
			switch (row->address_family()) {
			case ProtocolFamily::INET:
				counter[INET_COUNT]++;
				break;
			case ProtocolFamily::INET6:
				counter[INET6_COUNT]++;
				break;
			}

			switch (row->protocol()) {
			case ConnectionProtocol::PROTO_TCP:
				counter[TCP_COUNT]++;
				break;
			case ConnectionProtocol::PROTO_UDP:
				counter[UDP_COUNT]++;
				break;
			}
		}

		set_items({
			std::format(L"ALL: {}", counter[ALL_COUNT]),
			std::format(L"TCP: {}", counter[TCP_COUNT]),
			std::format(L"UDP: {}", counter[UDP_COUNT]),
			std::format(L"IPv4: {}", counter[INET_COUNT]),
			std::format(L"IPv6: {}", counter[INET6_COUNT]),
		});
	}

	~StatusBar() {
		if (m_sb) {
			DestroyWindow(m_sb);
			m_sb = NULL;
		}
	}

	int height() const { return m_height; }

private:
	int set_text(char index, LPCWSTR text) {
		WPARAM wParam = MAKEWPARAM(MAKEWORD(index, SBT_NOBORDERS), 0);
		return SendMessage(m_sb, SB_SETTEXT, wParam, (LPARAM)text);
	}

	void init() {
		RECT rcClient;
		int width, sb_width;

		GetClientRect(m_parent, &rcClient);
		sb_width = rcClient.right;
	
		if (sb_width > 1000) {
			width = (sb_width / 1000 * 1000) / m_parts;
		}
		else if (sb_width > 100) {
			width = (sb_width / 100 * 100) / m_parts;
		}
		else {
			width = (sb_width / 10 * 10) / m_parts;
		}
		
		std::vector<int> parts(m_parts, width);

		for (int i = 1; i < m_parts; i++) {
			parts[i] += parts[i - 1];
		}
		
		SendMessage(m_sb, SB_SETPARTS, (WPARAM)m_parts, (LPARAM)parts.data());
	}

	void get_sb_height() {
		RECT rcStatus;
		if (GetWindowRect(m_sb, &rcStatus))
		{
			m_height = rcStatus.bottom - rcStatus.top;
		}
		else {
			std::runtime_error("Failed to retrieve StatusBar height");
		}
	}

	HWND m_sb{ NULL };
	HWND m_parent{ NULL };
	int m_parts{ 5 };

	int m_height{-1};

	DWORD m_styles{ WS_CHILD };
};

#endif