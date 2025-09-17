#include "FindDlg.hpp"
#include "resource.h"

static INT_PTR CALLBACK FindDialogCallback(HWND hFind, UINT message, WPARAM wParam, LPARAM lParam);

static void SetFindDialogProp(HWND hFind, HWND hWnd) {
	SetProp(hFind, L"ListViewHandle", hWnd);
}

static HWND GetFindDlgParent(HWND hFind) {
	return (HWND)GetProp(hFind, L"ListViewHandle");
}

HWND InitFindDialog(HWND hWnd, const std::array<std::wstring, (int)Column::Count> &columns) {
	HWND hFindDlg = CreateDialog(NULL, MAKEINTRESOURCE(IDD_FINDBOX), hWnd, FindDialogCallback);

	SetFindDialogProp(hFindDlg, hWnd);

	// populate combobox
	HWND hComboBox = GetDlgItem(hFindDlg, IDC_SEARCHBY);

	for (const auto& str : columns) {
		ComboBox_AddString(hComboBox, str.c_str());
	}
	// set process name column by default for searching against to
	ComboBox_SetCurSel(hComboBox, 0);

	// turn radio on for 'Down'
	CheckRadioButton(hFindDlg, IDC_RADIO1, IDC_RADIO2, IDC_RADIO2);

	return hFindDlg;
}

static void DialogFindNextRow(
	HWND hFind,
	LPCWCHAR find_buf,
	SearchBy column,
	bool search_downwards,
	bool case_sensitive = false)
{
	constexpr int buf_len = 512;
	static WCHAR cell_buf[buf_len];

	HWND lv = GetFindDlgParent(hFind);

	int row_count = ListView_GetItemCount(lv);
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
		ListView_GetItemText(lv, i, (int)column, cell_buf, buf_len);
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
	ListView_SetItemState(lv, -1, 0, LVIS_SELECTED);

	ListView_SetItemState(lv, index, LVIS_SELECTED, LVIS_SELECTED);
	ListView_EnsureVisible(lv, index, TRUE); // scroll list view
}

static void DialogFindRow(HWND hFind) {
	constexpr int buf_len = 512;
	static WCHAR find_buf[buf_len];
	Column column;
	HWND hFindInput;
	bool search_downwards;

	search_downwards = IsDlgButtonChecked(hFind, IDC_RADIO2) ? true : false;

	column = (Column)ComboBox_GetCurSel(GetDlgItem(hFind, IDC_SEARCHBY));

	if (column < Column::ProcessName || column > Column::State) {
		return;
	}

	hFindInput = GetDlgItem(hFind, IDC_FINDINPUT);
	int find_buf_len = GetWindowText(hFindInput, find_buf, buf_len);

	if (!find_buf_len) {
		return;
	}

	CharLowerBuffW(find_buf, find_buf_len);

	DialogFindNextRow(hFind, find_buf, column, search_downwards);
}

static INT_PTR CALLBACK FindDialogCallback(HWND hFind, UINT message, WPARAM wParam, LPARAM lParam) {
	UNREFERENCED_PARAMETER(lParam);
	switch (message)
	{
	case WM_INITDIALOG:
		return (INT_PTR)TRUE;

	case WM_COMMAND:
	{
		int wID = LOWORD(wParam);
		switch (wID) {
		case IDFINDNEXT:
			DialogFindRow(hFind);
			break;
		case IDCANCEL:
			ShowWindow(hFind, HIDE_WINDOW);
			return (INT_PTR)TRUE;
		}
		break;
	}
	}
	return (INT_PTR)FALSE;
}