#ifndef LIST_VIEW_HPP
#define LIST_VIEW_HPP

#include <string>
#include <vector>
#include <stdexcept>

#include "framework.h"
#include "libTcpSpy/ConnectionsTableManager.hpp"

class ListView {
public:
	using pointer = std::unique_ptr<ListView>;

	ListView(HWND parent, ConnectionsTableManager &tblmngr)
		: m_parent(parent), m_mgr(tblmngr)
	{
		RECT rcClient;

		GetClientRect(parent, &rcClient);

		m_lv = CreateWindow(
			WC_LISTVIEW,
			L"",
			m_style,
			0, 0,
			rcClient.right - rcClient.left,
			rcClient.bottom - rcClient.top,
			m_parent,
			NULL,
			NULL,
			NULL);

		ListView_SetExtendedListViewStyle(m_lv, LVS_EX_AUTOSIZECOLUMNS | LVS_EX_FULLROWSELECT);

		init_image_list();
	};

	ListView(const ListView& lv) = delete;

	ListView(ListView&& lv) noexcept
		: m_lv(lv.m_lv), m_parent(lv.m_parent)
		, m_style(lv.m_style), m_image_list(lv.m_image_list), m_mgr(lv.m_mgr)
	{
		lv.m_parent = NULL;
		lv.m_lv = NULL;
		lv.m_image_list = NULL;

		lv.m_style = 0;
	}

	~ListView() {
		// i think destroying handles is not neccessary, because lifetime of listview is as long as main program's lifetime
	}

	// initializes imagelist's columns, their width, etc...
	// Sequence is an iterable sequence of std::wstring elements
	template<typename Sequence>
	void init_list(const Sequence &columns) {
		if (columns.empty()) {
			throw std::runtime_error("Empty columns passed");
		}

		LV_COLUMN lvColumn{};

		lvColumn.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
		lvColumn.fmt = LVCFMT_LEFT;
		lvColumn.cx = 180;
		lvColumn.pszText = const_cast<LPWSTR>(columns[0].data());

		ListView_InsertColumn(m_lv, 0, &lvColumn);

		lvColumn.cx = 100;
		for (int i = 1; i < columns.size(); i++) {
			lvColumn.pszText = const_cast<LPWSTR>(columns[i].data());

			ListView_InsertColumn(m_lv, i, &lvColumn);
		}
	}

	void refresh_items() {
		m_mgr.update();
		insert_items();
	}

	LPWSTR draw_column(int item, Column col) {
		auto& row = m_mgr.get()[item];

		constexpr auto BUF_LEN = 512;
		static WCHAR buf[BUF_LEN];
		std::wstring tmp;

		switch (col)
		{
		case Column::ProcessName:
			tmp = row->get_process_name();
			break;
		case Column::PID:
			tmp = row->pid_str();
			break;
		case Column::Protocol:
			switch (row->protocol()) {
			case ConnectionProtocol::PROTO_TCP: return (LPWSTR)L"TCP";
			case ConnectionProtocol::PROTO_UDP: return (LPWSTR)L"UDP";
			default:
				assert(false);
				break;
			}
			break;
		case Column::INET:
			switch (row->address_family()) {
			case ProtocolFamily::INET:  return (LPWSTR)L"IPv4";
			case ProtocolFamily::INET6: return (LPWSTR)L"IPv6";
			}
			break;
		case Column::LocalAddress:
			tmp = row->local_addr_str();
			break;
		case Column::LocalPort:
			tmp = row->local_port_str();
			break;
		case Column::RemoteAddress:
			if (auto p = dynamic_cast<ConnectionEntryTCP*>(row.get())) {
				tmp = p->remote_addr_str();
			}
			else {
				return nullptr;
			}
			break;
		case Column::RemotePort:
			if (auto p = dynamic_cast<ConnectionEntryTCP*>(row.get())) {
				tmp = p->remote_port_str();
			}
			else {
				return nullptr;
			}
			break;
		case Column::State:
			if (auto p = dynamic_cast<ConnectionEntryTCP*>(row.get())) {
				tmp = p->state_str();
			}
			else {
				return nullptr;
			}
			break;
		default:
			return nullptr;
		}

		HRESULT res = StringCchCopyW(buf, BUF_LEN, (LPWSTR)tmp.c_str());

		if (res == STRSAFE_E_INVALID_PARAMETER) {
			throw std::invalid_argument("The value in cchDest is either 0 or larger than STRSAFE_MAX_CCH");
		}

		return buf;
	}

	void sort_column(Column col) {
		static Column prev_clicked_column{};
		static bool asc = true;

		if (prev_clicked_column == col) {
			asc = !asc;
		}
		else {
			asc = true;
		}

		m_mgr.sort(col, asc);
		insert_items();

		prev_clicked_column = col;
	}

	void resize() {
		RECT rc;

		GetClientRect(m_parent, &rc);

		MoveWindow(m_lv,
			rc.left,
			rc.top,
			rc.right - rc.left,
			rc.bottom - rc.top,
			TRUE
		);
	}

	template<typename F>
	void set_subclass(F f) {
		SetWindowSubclass(m_lv, f, 0, 0);
	}
private:
	void init_image_list() {
		m_image_list = ImageList_Create(
			GetSystemMetrics(SM_CXSMICON),
			GetSystemMetrics(SM_CYSMICON),
			ILC_COLOR16, 1, 1);

		ListView_SetImageList(m_lv, m_image_list, LVSIL_SMALL);
	}

	void insert_items() {
		ListView_DeleteAllItems(m_lv);
		ImageList_RemoveAll(m_image_list);

		if (!m_mgr.size()) return;

		LVITEM item;
		ZeroMemory(&item, sizeof(item));

		item.pszText = LPSTR_TEXTCALLBACK;
		item.mask = LVIF_TEXT | LVIF_STATE | LVIF_IMAGE;
		item.state = 0;
		item.stateMask = 0;
		item.iSubItem = 0;

		for (int i = 0; i < m_mgr.size(); i++) {
			auto& row = m_mgr.get()[i];
			if (row->icon() == nullptr) {
				HICON default_icon = LoadIcon(NULL, MAKEINTRESOURCE(IDI_APPLICATION));
				ImageList_AddIcon(m_image_list, default_icon);
				DestroyIcon(default_icon);
			}
			else {
				ImageList_AddIcon(m_image_list, row->icon());
			}

			item.iItem = i;
			item.iImage = i;

			if (ListView_InsertItem(m_lv, &item) == -1) {
				return;
			}
		}
	}

	HWND m_parent;
	HWND m_lv;
	HIMAGELIST m_image_list;
	ConnectionsTableManager& m_mgr;
	DWORD m_style{ WS_TABSTOP | WS_CHILD | WS_BORDER | WS_VISIBLE | LVS_AUTOARRANGE | LVS_REPORT };
};

#endif
