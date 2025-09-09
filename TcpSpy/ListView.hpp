#ifndef LIST_VIEW_HPP
#define LIST_VIEW_HPP

#include <string>
#include <vector>
#include <stdexcept>

#include "framework.h"
#include "libTcpSpy/ConnectionsTableManager.hpp"
#include "PopupMenu.hpp"
#include "Shell.hpp"

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

		SetFocus(m_lv);

		m_popup_menu.insert_items({ L"Properties" });
	};

	ListView(const ListView& lv) = delete;

	ListView(ListView&& lv) noexcept
		: m_lv(lv.m_lv), m_parent(lv.m_parent)
		, m_style(lv.m_style), m_image_list(lv.m_image_list)
		, m_mgr(lv.m_mgr), m_popup_menu(std::move(lv.m_popup_menu))
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

	LPWSTR draw_cell(int item, Column col) {
		const auto& row = m_mgr[item];

		constexpr int BUF_LEN = 512;
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
			if (const auto p = dynamic_cast<ConnectionEntryTCP*>(row.get())) {
				tmp = p->remote_addr_str();
			}
			else {
				return nullptr;
			}
			break;
		case Column::RemotePort:
			if (const auto p = dynamic_cast<ConnectionEntryTCP*>(row.get())) {
				tmp = p->remote_port_str();
			}
			else {
				return nullptr;
			}
			break;
		case Column::State:
			if (const auto p = dynamic_cast<ConnectionEntryTCP*>(row.get())) {
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

	template<typename Func>
	void set_subclass(Func f) {
		SetWindowSubclass(m_lv, f, 0, 0);
	}

	void search(LPCWCHAR find_buf, SearchBy column, bool search_downwards) const {
		constexpr int buf_len = 512;
		static WCHAR cell_buf[buf_len];

		int row_count = ListView_GetItemCount(m_lv);
		static int index = -1; // should be saved for consequence calls
		int i;

		if (search_downwards) {
			if (index != -1 && index != row_count - 1) {
				i = index + 1;
			}
			else {
				i = 0;
			}
		}
		else {
			if (index != -1 && index != 0) {
				i = index - 1;
			}
			else {
				i = row_count - 1;
			}
		}

		for (;
			search_downwards ? (i < row_count) : (i >= 0);
			search_downwards ? i++ : i--
			) 
		{
			ListView_GetItemText(m_lv, i, (int)column, cell_buf, buf_len);

			// make case-insensitive search, maybe add case insensitive/sensitive search option
			int cell_buf_len = CharLowerBuffW(cell_buf, buf_len);

			LPCWSTR p1 = find_buf, p2 = cell_buf;

			while (*p1 == *p2) { p1++; p2++; }

			if (!*p1) {
				// whole find_buf string matched
				index = i;
				break;
			}
			else {
				index = -1;
			}
		}

		// didnt find just return
		if (index == -1) {
			return;
		}

		// clear selected rows if any
		ListView_SetItemState(m_lv, -1, 0, LVIS_SELECTED);

		ListView_SetItemState(m_lv, index, LVIS_SELECTED, LVIS_SELECTED);
		ListView_EnsureVisible(m_lv, index, TRUE); // scroll list view
	}

	HWND hwnd() const { return m_lv; }

	void show_popup() {
		POINT pt;

		GetCursorPos(&pt);

		int cmd = m_popup_menu.show(m_lv, pt.x, pt.y);

		if (!cmd && GetLastError()) {
			// error occured or item is not selected
			return;
		}

		switch (cmd) {
		case 1:
		{
			// if region is selected and right mouse button is pressed, choose the first selected row
			int selected_row = ListView_GetNextItem(m_lv, -1, LVNI_SELECTED);

			const auto& proc_path = m_mgr[selected_row]->proc().m_path;

			// start windows' 'properites' window
			Shell::Properties(m_lv, proc_path.c_str());
		}
		break;
		}
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
	DWORD m_style{ WS_TABSTOP | WS_CHILD | WS_BORDER | WS_VISIBLE | LVS_AUTOARRANGE | LVS_REPORT | LVS_SHOWSELALWAYS };
	PopupMenu m_popup_menu;
};

#endif
