#include "framework.h"
#include "TcpSpy.h"

#include <algorithm>

#include "libTcpSpy/ConnectionsTableManager.hpp"
#include "ListView.hpp"

#define MAX_LOADSTRING 100

HINSTANCE hInst;                                // current instance
WCHAR szTitle[MAX_LOADSTRING];                  // The title bar text
WCHAR szWindowClass[MAX_LOADSTRING];            // the main window class name

const std::array<std::wstring, 9> columns { 
	L"Process name", L"PID", L"Protocol",
	L"IP version", L"Local Address", L"Local Port",
	L"Remote Address", L"Remote Port", L"State",
};

ListView::pointer listView;
ConnectionsTableManager connectionsManager;

HWND hFindDlg;
HMENU Menu;

// key is ID of menu item, std::pair is filter with bool value turned on/off filter
std::unordered_map<int, std::pair<ConnectionsTableManager::Filters, bool>> MenuFilters{
	{ ID_TCP_LISTENER,   { ConnectionsTableManager::Filters::TCP_LISTENING,   true} },
	{ ID_TCP_CONNECTED,  { ConnectionsTableManager::Filters::TCP_CONNECTIONS, true} },
	{ ID_IPVERSION_IPV4, { ConnectionsTableManager::Filters::IPv4,            true} },
	{ ID_IPVERSION_IPV6, { ConnectionsTableManager::Filters::IPv6,            true} },
	{ ID_VIEW_UDP,       { ConnectionsTableManager::Filters::UDP,             true} },
};

ATOM             MyRegisterClass(HINSTANCE hInstance);
BOOL             InitInstance(HINSTANCE, int);
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK About(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK FindDialogCallback(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK ListViewSubclassProc(HWND, UINT, WPARAM, LPARAM, UINT_PTR, DWORD_PTR);

static void InitCommonCtrls();
static void InitWSA();
static void HandleWM_NOTIFY(LPARAM lParam);
static void CheckUncheckMenuItem(int menu_item_id, int flag);
static void ModFilter(ConnectionsTableManager::Filters filter, bool value);
static void OpenFindDialog();
static void InitListView(HWND hWnd);
static void InitFindDialog(HWND hWnd);
static void ChangeFilter(int id);

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
	_In_opt_ HINSTANCE hPrevInstance,
	_In_ LPWSTR    lpCmdLine,
	_In_ int       nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);
	
	InitWSA();
	InitCommonCtrls();
	
	LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
	LoadStringW(hInstance, IDC_TCPSPY, szWindowClass, MAX_LOADSTRING);

	MyRegisterClass(hInstance);

	if (!InitInstance(hInstance, nCmdShow))
	{
		return FALSE;
	}

	HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_TCPSPY));

	MSG msg;

	while (GetMessage(&msg, nullptr, 0, 0))
	{
		if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	WSACleanup();

	return (int)msg.wParam;
}

ATOM MyRegisterClass(HINSTANCE hInstance)
{
	WNDCLASSEXW wcex;

	wcex.cbSize = sizeof(WNDCLASSEX);

	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = WndProc;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = hInstance;
	wcex.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_TCPSPY));
	wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
	wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wcex.lpszMenuName = MAKEINTRESOURCEW(IDC_TCPSPY);
	wcex.lpszClassName = szWindowClass;
	wcex.hIconSm = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

	return RegisterClassExW(&wcex);
}

BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
	hInst = hInstance; // Store instance handle in our global variable

	HWND hWnd = CreateWindowW(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, nullptr, nullptr, hInstance, nullptr);

	if (!hWnd)
	{
		return FALSE;
	}

	ShowWindow(hWnd, nCmdShow);
	UpdateWindow(hWnd);

	return TRUE;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_CREATE:
	{
		// init windows that depend on main window, initialize them
		Menu = GetMenu(hWnd);
		InitFindDialog(hWnd);
		InitListView(hWnd);

	}
		break;
	case WM_COMMAND:
	{
		int wmId = LOWORD(wParam);
		switch (wmId)
		{
		case IDM_ABOUT:
			DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
			break;
		case ID_FINDBOX: /* Button from menu and shortcut */
		case ID_FILE_FIND:
			OpenFindDialog();
			break;
		case ID_VIEW_REFRESH:
		case ID_REFRESHF5:
			listView->update();
			break;
		// Process filters
		case ID_TCP_LISTENER:
		case ID_TCP_CONNECTED:
		case ID_IPVERSION_IPV4:
		case ID_IPVERSION_IPV6:
		case ID_VIEW_UDP:
			ChangeFilter(wmId);
			listView->update(); // refresh items after applied filters
			break;
		case ID_QUICKEXIT:
		case IDM_EXIT:
			DestroyWindow(hWnd);
			break;
		default:
			return DefWindowProc(hWnd, message, wParam, lParam);
		}
	}
	break;
	case WM_NOTIFY:
		HandleWM_NOTIFY(lParam);
		break;
	case WM_SIZE:
		listView->resize();
		break;
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}

INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);
	switch (message)
	{
	case WM_INITDIALOG:
		return (INT_PTR)TRUE;

	case WM_COMMAND:
		if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
		{
			EndDialog(hDlg, LOWORD(wParam));
			return (INT_PTR)TRUE;
		}
		break;
	}
	return (INT_PTR)FALSE;
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

	listView->search(find_buf, column, search_downwards);
}

INT_PTR CALLBACK FindDialogCallback(HWND hFind, UINT message, WPARAM wParam, LPARAM lParam) {
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
	case WM_KEYDOWN:
		// DOES NOT RECEIVE FOR SOME REASON
		if ((wParam == VK_RETURN) || (wParam == VK_TAB)) {
			return FALSE;
		}
		break;
	}
	return (INT_PTR)FALSE;
}

void InitCommonCtrls() {
	INITCOMMONCONTROLSEX icc{};
	icc.dwICC = ICC_LISTVIEW_CLASSES;
	InitCommonControlsEx(&icc);
}

static void CheckUncheckMenuItem(int menu_item_id, int flag) {
	CheckMenuItem(Menu, menu_item_id, MF_BYCOMMAND | (flag ? MF_CHECKED : MF_UNCHECKED));
}

static void ChangeFilter(int id) {
	bool& flag = MenuFilters[id].second;
	CheckUncheckMenuItem(id, (flag = !flag));
	ModFilter(MenuFilters[id].first, flag);
}

static void HandleWM_NOTIFY(LPARAM lParam) {
	switch (((NMHDR*)lParam)->code)
	{
	case LVN_GETDISPINFO:
	{
		NMLVDISPINFO* plvdi = (NMLVDISPINFO*)lParam;
		plvdi->item.pszText = listView->draw_cell(plvdi->item.iItem, (Column)plvdi->item.iSubItem);
	}
		break;
	case LVN_COLUMNCLICK:
	{
		LPNMLISTVIEW pnmv = (LPNMLISTVIEW)lParam;
		Column col = (Column)pnmv->iSubItem;

		if (col >= Column::ProcessName && col <= Column::State) {
			listView->sort_column(col);
		}
	}
		break;
	case NM_RCLICK:
	{
		POINT pt;
		GetCursorPos(&pt);
		listView->show_popup(pt);
	}
	break;
	case LVN_BEGINSCROLL:
		listView->resize();
	break;
	}
}

LRESULT CALLBACK ListViewSubclassProc(
	HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam,
	UINT_PTR uIdSubclass, DWORD_PTR dwRefData) 
{
	// Proc is used to handle specific 
	int wmId = LOWORD(wParam);
	switch (msg)
	{
	case WM_COMMAND:
		switch (wmId) {
		case IDM_ABOUT:
			DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
			break;
		case ID_FINDBOX: /* Button from menu and shortcut */
			OpenFindDialog();
			break;
		case ID_VIEW_REFRESH:
		case ID_REFRESHF5:
			listView->update();
			break;
		case ID_QUICKEXIT:
			DestroyWindow(GetParent(hWnd)); // send exit to main window
			break;
		}
		break;
	}
	return DefSubclassProc(hWnd, msg, wParam, lParam);
}

static void ModFilter(ConnectionsTableManager::Filters filter, bool value) {
	if (value) {
		connectionsManager.add_filter(filter);
	}
	else {
		connectionsManager.remove_filter(filter);
	}
}

static void OpenFindDialog() {
	ShowWindow(hFindDlg, SW_SHOW);
	SetFocus(GetDlgItem(hFindDlg, IDC_FINDINPUT)); // set focus to edit control for keyboard
}

static void InitWSA() {
	WSADATA wsaData;
	WORD wVersionRequested = MAKEWORD(2, 2);

	if (WSAStartup(wVersionRequested, &wsaData) != 0) {
		std::cerr << "WSAStartup failed" << std::endl;
		exit(1);
	}
}

static void InitListView(HWND hWnd) {
	listView = std::make_unique<ListView>(hWnd, connectionsManager);

	listView->set_subclass(ListViewSubclassProc);

	listView->init_list(columns);

	listView->update();
}

static void InitFindDialog(HWND hWnd) {
	hFindDlg = CreateDialog(NULL, MAKEINTRESOURCE(IDD_FINDBOX), hWnd, FindDialogCallback);

	// populate combobox
	HWND hComboBox = GetDlgItem(hFindDlg, IDC_SEARCHBY);

	for (const auto &str : columns) {
		ComboBox_AddString(hComboBox, str.c_str());
	}
	// set process name column by default for searching against to
	ComboBox_SetCurSel(hComboBox, Column::ProcessName);

	// turn radio on for 'Down'
	CheckRadioButton(hFindDlg, IDC_RADIO1, IDC_RADIO2, IDC_RADIO2);
}