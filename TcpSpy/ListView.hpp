#ifndef LIST_VIEW_HPP
#define LIST_VIEW_HPP

#include <string>
#include <vector>
#include <stdexcept>

#include "framework.h"
#include "libTcpSpy/ConnectionsTableManager.hpp"

enum ListViewColumns {
	PROC_NAME,
	PID,
	PROTOCOL,
	IPVERSION,
	LOC_ADDR,
	LOC_PORT,
	REM_ADDR,
	REM_PORT,
	STATE,
};

class ListView {
public:
	using pointer = std::unique_ptr<ListView>;

	ListView(HWND parent)
		: m_parent(parent)
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
		, m_style(lv.m_style), m_image_list(lv.m_image_list)
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
	void init_list(const std::vector<std::wstring>& columns) {
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

	template<typename T>
	void insert_items(T& src);

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

	HWND m_parent;
	HWND m_lv;
	HIMAGELIST m_image_list;
	DWORD m_style{ WS_TABSTOP | WS_CHILD | WS_BORDER | WS_VISIBLE | LVS_AUTOARRANGE | LVS_REPORT };
};

template<>
void ListView::insert_items<ConnectionsTableManager>(ConnectionsTableManager &mgr) {
	ListView_DeleteAllItems(m_lv);
	ImageList_RemoveAll(m_image_list);

	if (!mgr.size()) return;

	LVITEM item;
	ZeroMemory(&item, sizeof(item));

	item.pszText = LPSTR_TEXTCALLBACK;
	item.mask = LVIF_TEXT | LVIF_STATE | LVIF_IMAGE;
	item.state = 0;
	item.stateMask = 0;
	item.iSubItem = 0;

	for (int i = 0; i < mgr.size(); i++) {
		auto& row = mgr.get()[i];
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

#endif
