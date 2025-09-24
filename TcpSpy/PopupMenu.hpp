#ifndef POPUP_MENU_HPP
#define POPUP_MENU_HPP

#include <vector>
#include <string>

#include "framework.h"


class PopupMenu {
public:
	enum SelectedMenuItem {
		BEG,
		Copy,
		Properties,
	};

	PopupMenu()
	{
		m_menu = CreatePopupMenu();

		m_menu_info.cbSize = sizeof(m_menu_info);
		m_menu_info.fMask = MIIM_STRING | MIIM_ID;
	}

	PopupMenu(PopupMenu &&pm) noexcept
		: m_menu(pm.m_menu), m_menu_info(pm.m_menu_info)
	{
		pm.m_menu = NULL;
	}

	void set_items(const std::vector<std::wstring> &items) {
		int item_id = SelectedMenuItem::BEG+1;

		for (const auto& item : items) {
			m_menu_info.dwTypeData = (LPWSTR)item.c_str();
			m_menu_info.cch = item.size();
			m_menu_info.wID = item_id;

			InsertMenuItem(m_menu, item_id, false, &m_menu_info);

			item_id++;
		}
	}

	int show(HWND hWnd, int x, int y) const {
		return TrackPopupMenuEx(m_menu, TPM_LEFTALIGN | TPM_RETURNCMD | TPM_LEFTBUTTON, x, y, hWnd, NULL);
	}

	~PopupMenu() {
		if (m_menu) {
			DestroyMenu(m_menu);
			m_menu = NULL;
		}
	}
private:
	const std::vector<std::wstring> m_items;
	HMENU m_menu{ NULL };
	MENUITEMINFO m_menu_info{};
};


#endif