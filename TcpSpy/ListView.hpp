#ifndef LIST_VIEW_HPP
#define LIST_VIEW_HPP

#include <string>
#include <vector>
#include <stdexcept>
#include <format>

#include "framework.h"
#include "libTcpSpy/ConnectionsTableManager.hpp"
#include "libTcpSpy/DomainResolver.hpp"
#include "libTcpSpy/Column.hpp"

#include "PopupMenu.hpp"
#include "Cursor.hpp"

#include "Shell.hpp"
#include "Clipboard.hpp"
#include "StatusBar.hpp"

class ListView {
public:
	using pointer = std::unique_ptr<ListView>;

	ListView(HWND parent, ConnectionsTableManager &tblmngr)
		: m_parent(parent), m_mgr(tblmngr)
	{
		RECT rcClient;

		GetClientRect(parent, &rcClient);

		m_status_bar = std::make_unique<StatusBar>(parent);

		m_lv = CreateWindow(
			WC_LISTVIEW,
			L"",
			m_style,
			0, 0,
			rcClient.right - rcClient.left,
			rcClient.bottom - rcClient.top - m_status_bar->height(),
			m_parent,
			NULL,
			NULL,
			NULL);

		ListView_SetExtendedListViewStyle(m_lv, LVS_EX_AUTOSIZECOLUMNS | LVS_EX_FULLROWSELECT);

		init_image_list();

		SetFocus(m_lv);

		m_find_dlg = InitFindDialog(m_lv, { 
			L"Process name", L"PID", L"Protocol", 
			L"IP version", L"Local Address", L"Local Port", 
			L"Remote Address", L"Remote Port", L"State",
			});
	};

	ListView(const ListView& lv) = delete;
	ListView(ListView&& lv) = delete;

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

		m_columns = columns.size();

		LV_COLUMN lvColumn{};

		lvColumn.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
		lvColumn.fmt = LVCFMT_LEFT;
		lvColumn.cx = m_first_column_len;
		lvColumn.pszText = const_cast<LPWSTR>(columns[0].data());
		lvColumn.iSubItem = 0;

		ListView_InsertColumn(m_lv, 0, &lvColumn);

		lvColumn.cx = m_column_len;
		for (int i = 1; i < columns.size(); i++) {
			lvColumn.pszText = const_cast<LPWSTR>(columns[i].data());
			lvColumn.iSubItem = i;
			ListView_InsertColumn(m_lv, i, &lvColumn);
		}
	}

	void update() {
		m_mgr.update();
		insert_items();

		m_status_bar->update(m_mgr);
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
			rc.bottom - rc.top - m_status_bar->height(),
			TRUE
		);
		m_status_bar->resize();
	}

	template<typename Func>
	void set_subclass(Func f) {
		SetWindowSubclass(m_lv, f, 0, 0);
	}

	void show_popup(POINT pt) {
		int row = get_selected_row();
		if (row == -1) return; // if no row is selected, do not show popup menu

		POINT orig_pt = pt;

		ScreenToClient(m_lv, &pt);

		PopupMenu popup_menu;

		popup_menu.set_items({ L"Copy", L"WhoIs", L"Properties"});

		int cmd = popup_menu.show(m_lv, orig_pt.x, orig_pt.y);

		if (!cmd && GetLastError()) {
			// error occured or item is not selected
			return;
		}

		switch (cmd) {
		case PopupMenu::SelectedMenuItem::Copy:
		{
			Column col = (Column)get_selected_col(pt.x);

			WCHAR cell_buf[512];
			get_cell_text(row, (int)col, cell_buf, 512);

			if (!Clipboard::CopyStrToClipboard(cell_buf)) {
				throw std::runtime_error("Failed to copy buf to clipboard");
			}
		}
			break;
		case PopupMenu::SelectedMenuItem::Properties:
		{
			const auto& proc_path = m_mgr[row]->proc().m_path;
			// start windows' 'properites' window
			Shell::Properties(m_lv, proc_path.c_str());
		}
		break;
		case PopupMenu::SelectedMenuItem::WhoIs:
		{
			static bool task_running = false;
			auto &r = m_mgr[row];
			if (r->protocol() == ConnectionProtocol::PROTO_TCP) {

				gCurrentCursor = gWaitCursor;
				
				if (!task_running) {
					m_dr.resolve_domain(
						((ConnectionEntryTCP*)r.get())->remote_addr(),
						r->address_family(),
						[this](std::wstring& resolved_domain) {
							gCurrentCursor = gArrowCursor;
							// lambda will run inside thread, capture needed data here
							task_running = false;
							MessageBox(
								this->m_lv,
								resolved_domain.size() ?
								resolved_domain.c_str() : L"Failed to resolve", L"Who is?",
								MB_OK
							);
						});
				}
				task_running = true;
			}
		}
			break;
		}
	}

	void show_find_dlg() {
		ShowWindow(m_find_dlg, SW_SHOW);
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

		LVITEM item{};
		item.pszText = LPSTR_TEXTCALLBACK;
		item.mask = LVIF_TEXT | LVIF_IMAGE;

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

	int get_selected_row() const {
		return ListView_GetNextItem(m_lv, -1, LVNI_SELECTED);
	}

	int get_selected_col(int x) const {
		// can only determine col by x coordinate
		int sum = 0;
		for (int i = 0; i < m_columns; i++) {
			int col_size = ListView_GetColumnWidth(m_lv, i);
			sum += col_size;
			if (x < sum) {
				return i;
			}
		}
		assert(false);
	}

	void get_cell_text(int row, int col, LPWSTR buf, size_t buf_size) const {
		ListView_GetItemText(m_lv, row, col, buf, buf_size);
	}

	int m_columns{ -1 };
	const int m_first_column_len = 180;
	const int m_column_len = 100;

	HWND m_parent;
	HWND m_lv;
	HWND m_find_dlg;
	DWORD m_style{ WS_TABSTOP | WS_CHILD | WS_BORDER | WS_VISIBLE | LVS_AUTOARRANGE | LVS_REPORT | LVS_SHOWSELALWAYS | LVS_SINGLESEL };
	HIMAGELIST m_image_list;

	StatusBar::pointer m_status_bar;
	ConnectionsTableManager& m_mgr;
	DomainResolver m_dr;
};

#endif
