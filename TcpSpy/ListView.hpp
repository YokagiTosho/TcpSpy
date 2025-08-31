#ifndef LIST_VIEW_HPP
#define LIST_VIEW_HPP

#include <string>
#include <vector>
#include <stdexcept>



#include "framework.h"

class ListView {
public:
	using pointer = std::unique_ptr<ListView>;

	ListView(HINSTANCE inst, HWND parent)
		: m_inst(inst), m_parent(parent)
	{
		RECT rcClient;

		GetClientRect(parent, &rcClient);

		// Create the list-view window in report view with label editing enabled.
		m_lv = CreateWindow(
							WC_LISTVIEW,
							L"",
							m_style,
							0,0,
							rcClient.right - rcClient.left,
							rcClient.bottom - rcClient.top,
							m_parent,
							NULL,
							m_inst,
							NULL);

		ListView_SetExtendedListViewStyle(m_lv, LVS_EX_AUTOSIZECOLUMNS | LVS_EX_FULLROWSELECT);
	};

	ListView(const ListView &lv) = delete;

	ListView(ListView &&lv) noexcept 
		: m_lv(lv.m_lv), m_parent(lv.m_parent), m_style(lv.m_style), m_inst(lv.m_inst)
	{
		lv.m_inst = NULL;
		lv.m_parent = NULL;
		lv.m_lv = NULL;

		lv.m_style = 0;
	}

	void init_list(const std::vector<std::wstring> &columns) {
		if (columns.empty()) {
			throw std::runtime_error("Empty columns passed");
		}

		LV_COLUMN lvColumn {};

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

	void insert_items(int items_size) {
		ListView_DeleteAllItems(m_lv);

		LVITEM item;
		ZeroMemory(&item, sizeof(item));

		item.pszText = LPSTR_TEXTCALLBACK;
		item.mask = LVIF_TEXT | LVIF_STATE | LVIF_IMAGE;
		item.state = 0;
		item.stateMask = 0;
		item.iSubItem = 0;

		for (int i = 0; i < items_size; i++) {
			item.iItem = i;
			//item.iImage = i;

			if (ListView_InsertItem(m_lv, &item) == -1) {
				return;
			}
		}
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
private:
	HINSTANCE m_inst;
	HWND m_parent;
	HWND m_lv;
	DWORD m_style{WS_TABSTOP | WS_CHILD | WS_BORDER | WS_VISIBLE | LVS_AUTOARRANGE | LVS_REPORT};
};

#endif
