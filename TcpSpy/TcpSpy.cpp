#include "framework.h"
#include "TcpSpy.h"

#include <algorithm>
#include <strsafe.h>

#include "libTcpSpy/ConnectionsTableManager.hpp"
#include "ListView.hpp"

#define MAX_LOADSTRING 100

HINSTANCE hInst;                                // current instance
WCHAR szTitle[MAX_LOADSTRING];                  // The title bar text
WCHAR szWindowClass[MAX_LOADSTRING];            // the main window class name

ListView::pointer listView;
ConnectionsTableManager connetionsManager;

HMENU Menu;
static bool DisplayTCPConnections = true;
static bool DisplayTCPListeners = true;
static bool DisplayUDP = true;
static bool DisplayIPv4 = true;
static bool DisplayIPv6 = true;

ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);

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

static void InitCommonCtrls();
static void InitWSA();
static void HandleWM_NOTIFY(LPARAM lParam);
static void CheckUncheckMenuItem(int menu_item_id, int flag);

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

//
//   FUNCTION: InitInstance(HINSTANCE, int)
//
//   PURPOSE: Saves instance handle and creates main window
//
//   COMMENTS:
//
//        In this function, we save the instance handle in a global variable and
//        create and display the main program window.
//
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

//
//  FUNCTION: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE: Processes messages for the main window.
//
//  WM_COMMAND  - process the application menu
//  WM_PAINT    - Paint the main window
//  WM_DESTROY  - post a quit message and return
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{

	case WM_CREATE:
		// init windows that depend on main window, initialize them

		Menu = GetMenu(hWnd);

		listView = std::make_unique<ListView>(hWnd);

		listView->init_list({
		   L"Process name",
		   L"PID",
		   L"Protocol",
		   L"IP version",
		   L"Local address",
		   L"Local port",
		   L"Remote address",
		   L"Remote port",
		   L"State"
			});

		connetionsManager.update();

		listView->insert_items(connetionsManager);
		break;
	case WM_COMMAND:
	{
		int wmId = LOWORD(wParam);
		switch (wmId)
		{
		case IDM_ABOUT:
			DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
			break;
		case ID_VIEW_REFRESH:
			connetionsManager.update();
			listView->insert_items(connetionsManager);
			break;
		case ID_TCP_LISTENER:
			CheckUncheckMenuItem(ID_TCP_LISTENER, (DisplayTCPListeners = !DisplayTCPListeners));
			if (DisplayTCPListeners) {
				connetionsManager.add_filter(ConnectionsTableManager::Filters::TCP_LISTENING);
			}
			else {
				connetionsManager.remove_filter(ConnectionsTableManager::Filters::TCP_LISTENING);
			}
			break;
		case ID_TCP_CONNECTED:
			CheckUncheckMenuItem(ID_TCP_CONNECTED, (DisplayTCPConnections = !DisplayTCPConnections));
			if (DisplayTCPConnections) {
				connetionsManager.add_filter(ConnectionsTableManager::Filters::TCP_CONNECTIONS);
			}
			else {
				connetionsManager.remove_filter(ConnectionsTableManager::Filters::TCP_CONNECTIONS);
			}
			break;
		case ID_VIEW_UDP:
			CheckUncheckMenuItem(ID_VIEW_UDP, (DisplayUDP = !DisplayUDP));
			if (DisplayUDP) {
				connetionsManager.add_filter(ConnectionsTableManager::Filters::UDP);
			}
			else {
				connetionsManager.remove_filter(ConnectionsTableManager::Filters::UDP);
			}
			break;
		case ID_IPVERSION_IPV4:
			CheckUncheckMenuItem(ID_IPVERSION_IPV4, (DisplayIPv4 = !DisplayIPv4));
			if (DisplayIPv4) {
				connetionsManager.add_filter(ConnectionsTableManager::Filters::IPv4);
			}
			else {
				connetionsManager.remove_filter(ConnectionsTableManager::Filters::IPv4);
			}
			break;
		case ID_IPVERSION_IPV6:
			CheckUncheckMenuItem(ID_IPVERSION_IPV6, (DisplayIPv6 = !DisplayIPv6));
			if (DisplayIPv6) {
				connetionsManager.add_filter(ConnectionsTableManager::Filters::IPv6);
			}
			else {
				connetionsManager.remove_filter(ConnectionsTableManager::Filters::IPv6);
			}
			break;
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

void InitCommonCtrls() {
	INITCOMMONCONTROLSEX icc{};
	icc.dwICC = ICC_LISTVIEW_CLASSES;
	InitCommonControlsEx(&icc);
}

static void CheckUncheckMenuItem(int menu_item_id, int flag) {
	CheckMenuItem(Menu, menu_item_id, MF_BYCOMMAND | (flag ? MF_CHECKED : MF_UNCHECKED));
}

static void ProcessListViewEntry(LPARAM lParam) {
	NMLVDISPINFO* plvdi = (NMLVDISPINFO*)lParam;

	auto &row = connetionsManager.get()[plvdi->item.iItem];

	constexpr auto BUF_LEN = 512;
	static WCHAR buf[BUF_LEN];
	std::wstring tmp;

	switch (plvdi->item.iSubItem)
	{
	case ListViewColumns::PROC_NAME:
		tmp = row->get_process_name();
		break;
	case ListViewColumns::PID:
		tmp = row->pid_str();
		break;
	case ListViewColumns::PROTOCOL:
		switch (row->protocol()) {
		case ConnectionProtocol::PROTO_TCP:
			plvdi->item.pszText = (LPWSTR)L"TCP";
			break;
		case ConnectionProtocol::PROTO_UDP:
			plvdi->item.pszText = (LPWSTR)L"UDP";
			break;
		default:
			assert(false);
			break;
		}
		return;
	case ListViewColumns::IPVERSION:
		switch (row->address_family()) {
		case ProtocolFamily::INET:
			plvdi->item.pszText = (LPWSTR)L"IPv4";
			break;
		case ProtocolFamily::INET6:
			plvdi->item.pszText = (LPWSTR)L"IPv6";
			break;
		}
		return; // return so plvdi->item.pszText is not assigned at the end of function
	case ListViewColumns::LOC_ADDR:
		tmp = row->local_addr_str();
		break;
	case ListViewColumns::LOC_PORT:
		tmp = row->local_port_str();
		break;
	case ListViewColumns::REM_ADDR:
		if (auto p = dynamic_cast<ConnectionEntryTCP*>(row.get())) {
			tmp = p->remote_addr_str();
		}
		else {
			return;
		}
		break;
	case ListViewColumns::REM_PORT:
		if (auto p = dynamic_cast<ConnectionEntryTCP*>(row.get())) {
			tmp = p->remote_port_str();
		}
		else {
			return;
		}
		break;
	case ListViewColumns::STATE:
		if (auto p = dynamic_cast<ConnectionEntryTCP*>(row.get())) {
			tmp = p->state_str();
		}
		else {
			return;
		}
		break;
	default:
		return;
	}

	HRESULT res = StringCchCopyW(buf, BUF_LEN, (LPWSTR)tmp.c_str());

	if (res == STRSAFE_E_INVALID_PARAMETER) {
		throw std::invalid_argument("The value in cchDest is either 0 or larger than STRSAFE_MAX_CCH");
	}

	plvdi->item.pszText = buf;
}

static void SortColumn(LPARAM lParam) {
	LPNMLISTVIEW pnmv = (LPNMLISTVIEW)lParam;

	if (pnmv->iSubItem < 0 || pnmv->iSubItem > ListViewColumns::STATE) {
		// out of bounds
		return;
	}

	static DWORD prev_clicked_column = -1;
	static bool asc = true;

	if (prev_clicked_column == pnmv->iSubItem) {
		asc = !asc;
	}
	else {
		asc = true;
	}

	connetionsManager.sort((SortBy)pnmv->iSubItem, asc);

	listView->insert_items(connetionsManager);

	prev_clicked_column = pnmv->iSubItem;
}

static void HandleWM_NOTIFY(LPARAM lParam) {
	switch (((NMHDR*)lParam)->code)
	{
	case LVN_GETDISPINFO:
		ProcessListViewEntry(lParam);
		break;
	case LVN_COLUMNCLICK:
		SortColumn(lParam);
		break;
	case NM_RCLICK:
		break;
	}
}

static void InitWSA() {
	WSADATA wsaData;
	WORD wVersionRequested = MAKEWORD(2, 2);

	if (WSAStartup(wVersionRequested, &wsaData) != 0) {
		std::cerr << "WSAStartup failed" << std::endl;
		exit(1);
	}
}